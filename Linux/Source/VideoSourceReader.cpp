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

//#define ASYNC_INVOKE

#ifdef USE_CUDA
#include "cuda_runtime.h"
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
    auto mbuffers = (it->second).mbuffers;

    //request buffer
    v4l2_requestbuffers req;
    std::memset(&req,0,sizeof(req));
    req.count = BUFFER_SIZE;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifdef USE_V4L2_USER_POINTER
    req.memory = V4L2_MEMORY_USERPTR;
#else
    req.memory = V4L2_MEMORY_MMAP;
#endif

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

    //query buffer , mmap , and enqueue  afterwards
    for(auto i = 0u; i < BUFFER_SIZE; ++i)
    {
#ifdef USE_V4L2_USER_POINTER
    #ifdef USE_CUDA
        if (cudaMallocHost(&mbuffers[i], (it->second).imageSize,
                           cudaHostAllocPortable|cudaHostAllocMapped)!=cudaSuccess)
            throw std::runtime_error("cudaMallocHost failed!");
	#else
    	mbuffers[i]= new uint8_t[(it->second).imageSize];
	#endif
        ReleaseFrame(handle, i);
#else
        v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(handle, VIDIOC_QUERYBUF, &buf))
            fprintf(stderr, "xxx VIDIOC_QUERYBUF  %s\n", strerror(errno));
        void *ptr = mmap(nullptr, // start anywhere
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
#endif
    }//for
}

void VideoSourceReader::Uninitmmap(uint32_t handle)
{
    auto it = streams.find(handle);
    if (it == streams.end())
        return;

    for(int i = 0; i < BUFFER_SIZE; ++i)
    {
        ReleaseFrame(handle, i);
#ifdef USE_V4L2_USER_POINTER
    #ifdef USE_CUDA
        cudaFreeHost((it->second).mbuffers[i]);
	#else
        delete[] (it->second).mbuffers[i];
	#endif
#else
        munmap((it->second).mbuffers[i], (it->second).imageSize);
#endif
        (it->second).mbuffers[i] = nullptr;

    }
}

std::shared_ptr<IVideoFrame> VideoSourceReader::RequestFrame(int handle, int &index) //DQBUF
{
    auto mbuffers = streams[handle].mbuffers;

    v4l2_buffer queue_buf;
    std::memset(&queue_buf,0,sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifdef USE_V4L2_USER_POINTER
    queue_buf.memory = V4L2_MEMORY_USERPTR;
#else
    queue_buf.memory = V4L2_MEMORY_MMAP;
#endif
    //queue_buf.flags = V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN;

    index = -1;

    if(ioctl(handle, VIDIOC_DQBUF, &queue_buf) == -1)
        return {};

    if (!streams[handle].lengthMutable &&
            ((queue_buf.flags & V4L2_BUF_FLAG_ERROR)!=0 || queue_buf.bytesused != queue_buf.length))
    {
        ReleaseFrame(handle, queue_buf.index);
        ++streams[handle].frameCounter;
        return {};
    }

    auto tm = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch());

    index = queue_buf.index;
    std::shared_ptr<IVideoFrame> frame(new VideoBufferLock(
                                           handle,
										   index,
                                           mbuffers[index],
                                           tm.count(),
                                           streams[handle].frameCounter,
                                           streams[handle].defaultStride,
                                           streams[handle].formats[streams[handle].currentFormatIndex],
                                           queue_buf.bytesused));
    ++streams[handle].frameCounter;
    return frame;
}

bool VideoSourceReader::ReleaseFrame(int handle, int index)
{
    v4l2_buffer queue_buf;
    std::memset(&queue_buf, 0, sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.index = index;
#ifdef USE_V4L2_USER_POINTER
    queue_buf.memory = V4L2_MEMORY_USERPTR;
    queue_buf.m.userptr = (unsigned long)(streams[handle].mbuffers[index]);
    queue_buf.length = streams[handle].imageSize;
#else
    queue_buf.memory = V4L2_MEMORY_MMAP;
#endif
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
                if (frameval.type==V4L2_FRMIVAL_TYPE_DISCRETE)
                    rate = (int)(frameval.discrete.denominator/frameval.discrete.numerator);
                else
                    rate = (int)(frameval.stepwise.max.denominator/frameval.stepwise.max.numerator);
                VideoFormat format;
                format.Width = width;
                format.Height = height;
                format.MaxRate = rate;
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

    auto &format = streams[handle].formats[streams[handle].currentFormatIndex];

    streams[handle].lengthMutable =
            std::memcmp(format.PixelFormat, "MJPG", 4)==0 || std::memcmp(format.PixelFormat, "H264", 4)==0;

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
        //Release unused frames
        for(int i=0;i<BUFFER_SIZE;++i)
            if (framesRef[i].second && framesRef[i].first.expired())
            {
                framesRef[i].second = false;
                ReleaseFrame(handle, i);
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
            //Add frame for watching
            framesRef[index].second = true;
            framesRef[index].first = frame;

            //Invoke callback handler async
            if (streams[handle].fncb)
            {
#ifdef ASYNC_INVOKE
                result = std::async(std::launch::async, [&](std::shared_ptr<IVideoFrame> f)
                {
                	streams[handle].fncb(f);
                }, frame);
#else
                streams[handle].fncb(frame);
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
