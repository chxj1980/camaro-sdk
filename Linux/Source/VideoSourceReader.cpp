#include "VideoSourceReader.h"

#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>

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
    {
        StopStream(item.first);
    }
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

    //request buffer
    v4l2_requestbuffers req;
    std::memset(&req,0,sizeof(req));
    req.count = FRAMEQUEUE_SIZE;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(handle, VIDIOC_REQBUFS, &req)==-1)
    {
        fprintf(stderr, "xxx VIDIOC_REQBUFS fail %s\n", strerror(errno));
    }

    //query buffer , mmap , and enqueue  afterwards
    for(auto i = 0; i < FRAMEQUEUE_SIZE; ++i)
    {
        //int j = i + FRAMEQUEUE_SIZE * handle;
        v4l2_buffer buf;
        std::memset(&buf,0,sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if(-1 == ioctl(handle, VIDIOC_QUERYBUF, &buf))
        {
            fprintf(stderr, "xxx VIDIOC_QUERYBUF  %s\n", strerror(errno));
        }
        void *ptr = mmap(NULL, // start anywhere
                       buf.length,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       handle, buf.m.offset);

        if(ptr == MAP_FAILED)
        {
            fprintf(stderr, "mmap fail %s ", strerror(errno));
            vbuffers[i].first = nullptr;
            vbuffers[i].second = 0;
        }
        else
        {
            vbuffers[i].first = reinterpret_cast<uint8_t *>(ptr);
            vbuffers[i].second = buf.length;
            if (ioctl(handle, VIDIOC_QBUF, &buf)<0)//enqueue
            {
                fprintf(stderr,"xxx VIDIOC_QBUF %s\n", strerror(errno));
            }
        }
    }//for
}

void VideoSourceReader::Uninitmmap(uint32_t handle)
{
    auto it = streams.find(handle);
    if (it == streams.end())
        return;
    for(int i = 0; i < FRAMEQUEUE_SIZE; ++i)
    {
        munmap((it->second).vbuffers[i].first, (it->second).vbuffers[i].second);
    }
}

std::shared_ptr<IVideoFrame> VideoSourceReader::RequestFrame(int handle, int &index) //DQBUF
{
    auto vbuffers = streams[handle].vbuffers;

    v4l2_buffer queue_buf;
    std::memset(&queue_buf,0,sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    queue_buf.flags = V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN;

    if(ioctl(handle, VIDIOC_DQBUF, &queue_buf) == -1)
    {
        index = -1;
        return {};
    }
    index = queue_buf.index;
    std::shared_ptr<IVideoFrame> frame(new VideoBufferLock(
                                       handle,
                                       queue_buf.index,
                                       vbuffers[queue_buf.index].first,
                                       queue_buf.timestamp,
                                       streams[handle].defaultStride,
                                       streams[handle].formats[streams[handle].currentFormatIndex]));

    return frame;
}

bool VideoSourceReader::ReleaseFrame(int handle, int index)
{
    v4l2_buffer queue_buf;
    std::memset(&queue_buf,0,sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    queue_buf.index = index;

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
            int rate, width, height;
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
                VideoFormat format = { width,height,rate,0 };
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
    Initmmap(handle);

    auto format = streams[handle].formats[streams[handle].currentFormatIndex];

    streams[handle].defaultStride =
            streams[handle].vbuffers[0].second / format.Height;
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
    int index;
    while (streams[handle].isRunning)
    {
        auto frame = RequestFrame(handle, index);
        if (frame && streams[handle].fncb)
        {
            //Invoke callback handler
            streams[handle].fncb(frame);
            ReleaseFrame(handle, index); //Release immediately if no callback
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

//bool VideoSourceReader::IsFormatSupported(const GUID &subtype) const
//{
//	//Do we support this type directly ?
//	return true;
//}
