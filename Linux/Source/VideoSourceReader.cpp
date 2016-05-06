#include "VideoSourceReader.h"

#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <future>
#include <chrono>

#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

#include "VideoBufferLock.h"
#include "VideoSourceProxy.h"
#include "v4l2helper.h"

using namespace TopGear;
using namespace Linux;

//#define USER_POINTER
//#define ASYNC_INVOKE

#ifdef __ARM_NEON__
void __attribute__ ((noinline)) neonMemCopy_gas(unsigned char* dst, unsigned char* src, int num_bytes)
{
    asm(
    "neoncopypld:\n"
        "       pld         [r1, #0xC0]\n"
        "       vldm        r1!,{d0-d7}\n"
        "       vstm        r0!,{d0-d7}\n"
        "       subs        r2,r2,#0x40\n"
        "       bge         neoncopypld\n"
    );
}
#endif

std::vector<std::shared_ptr<IVideoStream>> VideoSourceReader::CreateVideoStreams(std::shared_ptr<IGenericVCDevice> &device)
{
    auto source = std::dynamic_pointer_cast<LSource>(device->GetSource());
    if (source == nullptr)
        return{};
    std::shared_ptr<VideoSourceReader> pPlayer(new VideoSourceReader(device));
    pPlayer->EnumerateStreams(*source);
    std::vector<std::shared_ptr<IVideoStream>> list;
    for (auto &s : pPlayer->streams)
        list.emplace_back(std::make_shared<VideoSourceProxy>(
            std::static_pointer_cast<IMultiVideoSource>(pPlayer), s.first));
    return list;
}

std::shared_ptr<IVideoStream> VideoSourceReader::CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device)
{
    auto source = std::dynamic_pointer_cast<LSource>(device->GetSource());
    if (source == nullptr)
        return{};
    auto handle = source->GetHandle();
    std::shared_ptr<VideoSourceReader> pPlayer(new VideoSourceReader(device));
    pPlayer->streams[handle] = StreamState {};
    pPlayer->EnumerateFormats(handle);
    return std::make_shared<VideoSourceProxy>(
                std::static_pointer_cast<IMultiVideoSource>(pPlayer), uint32_t(handle));
}

//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

VideoSourceReader::VideoSourceReader(std::shared_ptr<IGenericVCDevice> &device)
    : deviceBackup(device)
{
}

//-------------------------------------------------------------------
//  destructor
//-------------------------------------------------------------------

VideoSourceReader::~VideoSourceReader()
{
    for(auto &item: streams)
        StopStream(item.first);
}

void VideoSourceReader::EnumerateStreams(const LSource &one)
{
    std::vector<SourcePair> sources;
    v4l2_capability cap;

    auto businfo = one.GetCapability().bus_info;

    v4l2Helper::EnumVideoDeviceSources(sources);
    for(auto &i: sources)
    {
        ioctl(i.second,VIDIOC_QUERYCAP,&cap);
        if (std::memcmp(cap.bus_info, businfo, 32)==0)
        {
            streams[i.second] = StreamState {};
            EnumerateFormats(i.second);
        }
    }
}

void VideoSourceReader::Initmmap(uint32_t handle)
{
    auto it = streams.find(handle);
    if (it ==streams.end())
        return;

    auto vbuffers = (it->second).vbuffers;
    auto mbuffers = (it->second).mbuffers;

    //request buffer
    v4l2_requestbuffers req;
    std::memset(&req,0,sizeof(req));
    req.count = BUFFER_SIZE;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(handle, VIDIOC_REQBUFS, &req) == -1)
        fprintf(stderr, "xxx VIDIOC_REQBUFS fail %s\n", strerror(errno));

    v4l2_format fmt;
    std::memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(handle, VIDIOC_G_FMT, &fmt);
    /* Buggy driver paranoia. */
    auto min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    (it->second).defaultStride = fmt.fmt.pix.bytesperline;
    (it->second).imageSize = fmt.fmt.pix.sizeimage;
//#endif

    for(auto i = 0u; i < FRAMEQUEUE_SIZE; ++i)
        vbuffers[i] = new uint8_t[(it->second).imageSize];

    //query buffer , mmap , and enqueue  afterwards
    for(auto i = 0u; i < BUFFER_SIZE; ++i)
    {
        v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(handle, VIDIOC_QUERYBUF, &buf))
            fprintf(stderr, "xxx VIDIOC_QUERYBUF  %s\n", strerror(errno));
        void *ptr = mmap(NULL, // start anywhere
                       buf.length,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       handle, buf.m.offset);

        if (ptr == MAP_FAILED)
        {
            fprintf(stderr, "mmap fail %s ", strerror(errno));
            mbuffers[i] = nullptr;
        }
        else
        {
            mbuffers[i] = reinterpret_cast<uint8_t *>(ptr);
            if (ioctl(handle, VIDIOC_QBUF, &buf)<0)     //enqueue
                fprintf(stderr,"xxx VIDIOC_QBUF %s\n", strerror(errno));
        }
//#endif
    }//for
}

void VideoSourceReader::Uninitmmap(uint32_t handle)
{
    auto it = streams.find(handle);
    if (it == streams.end())
        return;

    (it->second).rmap.Clear();

    for(int i = 0; i < FRAMEQUEUE_SIZE; ++i)
    {
        delete[] (it->second).vbuffers[i];
        (it->second).vbuffers[i] = nullptr;
    }

    for(int i = 0; i < BUFFER_SIZE; ++i)
    {
        munmap((it->second).mbuffers[i], (it->second).imageSize);
        (it->second).mbuffers[i] = nullptr;
    }

    //request buffer
    v4l2_requestbuffers req;
    std::memset(&req,0,sizeof(req));
    req.count = 0;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(handle, VIDIOC_REQBUFS, &req) == -1)
        fprintf(stderr, "xxx VIDIOC_REQBUFS fail %s\n", strerror(errno));
}

std::shared_ptr<IVideoFrame> VideoSourceReader::RequestFrame(int handle, int &index) //DQBUF
{
    auto vbuffers = streams[handle].vbuffers;
    auto mbuffers = streams[handle].mbuffers;

    v4l2_buffer queue_buf;
    std::memset(&queue_buf,0,sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    queue_buf.flags = V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN;

    index = -1;

    if(ioctl(handle, VIDIOC_DQBUF, &queue_buf) == -1)
        return {};

    auto tm = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch());

    for(int i=0;i<FRAMEQUEUE_SIZE;++i)
        if (!streams[handle].framesRef[i].second)
        {
            index = i;
            break;
        }
    if (index<0)    //unavailable buffer
        return {};

    //Copy mmap buffer to user buffer
#ifdef __ARM_NEON__
    neonMemCopy_gas(vbuffers[index], mbuffers[queue_buf.index], queue_buf.length);
#else
    std::memcpy(vbuffers[index], mbuffers[queue_buf.index], queue_buf.length);
#endif
    ReleaseFrame(handle, queue_buf.index);

    std::shared_ptr<IVideoFrame> frame(new VideoBufferLock(
                                       handle,
                                       index,
                                       vbuffers[index],
                                       tm.count(),
                                       streams[handle].frameCounter,
                                       streams[handle].defaultStride,
                                       streams[handle].formats[streams[handle].currentFormatIndex]));

    streams[handle].rmap.Register(index, frame, tm.count(), streams[handle].frameCounter++);

    ++streams[handle].frameCounter;
    return frame;
}

bool VideoSourceReader::ReleaseFrame(int handle, int index)
{
    v4l2_buffer queue_buf;
    std::memset(&queue_buf, 0, sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.index = index;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    return ioctl(handle, VIDIOC_QBUF, &queue_buf)==0;
}

void VideoSourceReader::EnumerateFormats(uint32_t handle)
{
    auto it = streams.find(handle);
    if (it ==streams.end())
        return;

    auto &videoFormats = (it->second).formats;

    videoFormats.clear();

    v4l2_fmtdesc fmt;
    std::memset(&fmt,0,sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while(ioctl(handle, VIDIOC_ENUM_FMT, &fmt) == 0)
    {
        v4l2_frmsizeenum fsize;
        std::memset(&fsize,0,sizeof(fsize));
        fsize.index=0;
        fsize.pixel_format = fmt.pixelformat;
        fsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while(ioctl(handle, VIDIOC_ENUM_FRAMESIZES, &fsize) == 0)
        {
            int rate, width=0, height=0;
            // Copy the frame size attributes
            if(fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                width = fsize.discrete.width;
                height = fsize.discrete.height;
            }
            else if(fsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
            {
                width = fsize.stepwise.max_width;
                height = fsize.stepwise.max_height;
            }
            else if(fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
            {
                width = fsize.stepwise.max_width;
                height = fsize.stepwise.max_height;
            }
            v4l2_frmivalenum frameval;
            std::memset(&frameval,0,sizeof(frameval));
            frameval.index = 0;
            frameval.pixel_format = fmt.pixelformat;
            frameval.width = width;
            frameval.height = height;
            while(ioctl(handle, VIDIOC_ENUM_FRAMEINTERVALS, &frameval)==0)
            {
                rate = (int)(frameval.discrete.denominator/frameval.discrete.numerator);
                VideoFormat format { width,height,rate,0 };
                memcpy(format.PixelFormat, &fmt.pixelformat, 4);
                videoFormats.emplace_back(format);
                frameval.index++;
            }
            fsize.index++;
        }
        fmt.index++;
    }
}

bool VideoSourceReader::SetCurrentFormat(uint32_t handle, int formatIndex)
{
    auto &videoFormats = streams[handle].formats;

    if (formatIndex<0 || formatIndex>=(int)videoFormats.size())
        return false;
    //currentFormatIndex = formatIndex;
    VideoFormat& format = videoFormats[formatIndex];
    streams[handle].currentFormatIndex = formatIndex;

    //Set frame format
    v4l2_format fmt;
    std::memset(&fmt,0,sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = format.Width;
    fmt.fmt.pix.height      = format.Height;
    memcpy(&fmt.fmt.pix.pixelformat, format.PixelFormat, 4);
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    ioctl(handle, VIDIOC_S_FMT, &fmt);

    //Set frame rate
    v4l2_streamparm param;
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //ioctl(handle, VIDIOC_G_PARM, &param);
    param.parm.capture.timeperframe.denominator = format.MaxRate;
    param.parm.capture.timeperframe.numerator = 1;
    ioctl(handle, VIDIOC_S_PARM, &param);

    return true;
}

const std::vector<VideoFormat> &VideoSourceReader::GetAllFormats(uint32_t handle)
{
    return streams[handle].formats;
}

bool VideoSourceReader::StartStream(uint32_t handle)
{
    if (streams.find(handle) == streams.end())
        return false;
    StopStream(handle);

    if (streams[handle].currentFormatIndex<0)
        return false;

    Initmmap(handle);

    streams[handle].isRunning = true;
    streams[handle].streamThread = std::thread(&VideoSourceReader::OnReadWorker, this, handle);
    streams[handle].streamOn = streams[handle].streamThread.joinable();
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(handle, VIDIOC_STREAMON, &type);
    return true;
}

bool VideoSourceReader::StopStream(uint32_t handle)
{
    if (streams.find(handle) == streams.end())
        return false;
    if (!streams[handle].isRunning)
        return false;
    if (streams[handle].streamThread.joinable())
    {
        streams[handle].isRunning = false;
        streams[handle].streamThread.join();
        streams[handle].streamOn = false;
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(handle, VIDIOC_STREAMOFF, &type);
    Uninitmmap(handle);
    return true;
}

bool VideoSourceReader::IsStreaming(uint32_t handle)
{
    if (streams.find(handle) == streams.end())
        return false;
    return streams[handle].streamOn;
}


void VideoSourceReader::OnReadWorker(uint32_t handle)
{
    int index = -1;
    auto framesRef = streams[handle].framesRef;
#ifdef ASYNC_INVOKE
    std::future<void> result;
#endif
    while (streams[handle].isRunning)
    {
        //Check mmap and release used frame
        for(int i=0;i<FRAMEQUEUE_SIZE;++i)
            if (framesRef[i].second && framesRef[i].first.expired())
            {
                streams[handle].rmap.Unregister(i);
                framesRef[i].second = false;
            }
#ifdef ASYNC_INVOKE
        //Check callback completion
        if (result.valid())
        {
            auto status = result.wait_for(std::chrono::milliseconds(0));
            if (status!=std::future_status::ready)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }
#endif
        auto frame = RequestFrame(handle, index);
        if (frame && index>=0)
        {
            framesRef[index].second = true;
            framesRef[index].first = frame;

            //Invoke callback handler async
            if (streams[handle].fncb)
            {
#ifdef ASYNC_INVOKE
                std::promise<bool> prom;
                std::future<bool> fut = prom.get_future();
                result = std::async(std::launch::async, [&]()
                {
                    auto f = framesRef[index].first.lock();
                    prom.set_value(true);
                    streams[handle].fncb(f);
                });
                fut.get();
#else
                auto f = framesRef[index].first.lock();
                streams[handle].fncb(f);
#endif
            }
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


void VideoSourceReader::RegisterReaderCallback(uint32_t handle, const ReaderCallbackFn& fn)
{
    auto it = streams.find(handle);
    if (it ==streams.end())
        return;
    (it->second).fncb = fn;
}
