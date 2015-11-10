#include "VideoSourceReader.h"

#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include "VideoBufferLock.h"
#include <thread>

#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;


//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

VideoSourceReader::VideoSourceReader(std::shared_ptr<ILSource> &source) :
    handleSource(source)
{
    handle = handleSource->GetSource();
    EnumerateFormats();
}

//-------------------------------------------------------------------
//  destructor
//-------------------------------------------------------------------

VideoSourceReader::~VideoSourceReader()
{
    StopStream();
}

void VideoSourceReader::Initmmap()
{
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

void VideoSourceReader::Uninitmmap()
{
    for(int i = 0; i < FRAMEQUEUE_SIZE; ++i)
    {
        munmap(vbuffers[i].first, vbuffers[i].second);
    }
}

std::shared_ptr<IVideoFrame> VideoSourceReader::RequestFrame(int &index) //DQBUF
{
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
                                                defaultStride, frameWidth, frameHeight));

    return frame;
}

bool VideoSourceReader::ReleaseFrame(int index)
{
    v4l2_buffer queue_buf;
    std::memset(&queue_buf,0,sizeof(queue_buf));
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    queue_buf.index = index;

    return ioctl(handle, VIDIOC_QBUF, &queue_buf)==0;
}

void VideoSourceReader::EnumerateFormats()
{
    videoFormats.clear();
    if (handle < 0)
		return;

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

int VideoSourceReader::GetMatchedFormatIndex(const VideoFormat& format) const
{
    if (handle<0)
		return -1;
	auto index = -1;
	for (auto i : videoFormats)
	{
		index++;
		if (format.Width > 0 && format.Width != i.Width)
			continue;
		if (format.Height> 0 && format.Height != i.Height)
			continue;
		if (format.MaxRate > 0 && format.MaxRate != i.MaxRate)
			continue;
        if (std::strcmp(format.PixelFormat, "")!=0 && std::strncmp(format.PixelFormat, i.PixelFormat, 4)!=0)
			continue;
		return index;
	}
	return -1;
}


int VideoSourceReader::GetOptimizedFormatIndex(VideoFormat& format, const char *fourcc)
{
    if (handle<0)
		return -1;

    //auto wCurrent = 0
    auto hCurrent = 0, rCurrent = 0;
	auto index = -1;
	auto i = -1;
	for(auto f : videoFormats)
	{
		++i;
		if (strcmp(fourcc, "") != 0 && strncmp(fourcc, f.PixelFormat, 4) != 0)
			continue;
		if (f.Height >= hCurrent && f.MaxRate >= rCurrent)
		{
            //wCurrent = f.Width;
			hCurrent = f.Height;
			rCurrent = f.MaxRate;
			index = i;
		}
	}
	if (index >= 0)
		format = videoFormats[index];
	return index;
}

bool VideoSourceReader::StartStream(int formatIndex)
{
	StopStream();
    if (formatIndex<0 || formatIndex>=(int)videoFormats.size())
        return false;
    currentFormatIndex = formatIndex;
    VideoFormat& format = videoFormats[formatIndex];
    frameHeight = format.Height;
    frameWidth = format.Width;

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

    Initmmap();
    defaultStride = vbuffers[0].second / frameHeight;
    isRunning = true;
    streamThread = std::thread(&VideoSourceReader::OnReadWorker,this);
    streamOn = streamThread.joinable();
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(handle, VIDIOC_STREAMON, &type);
    return true;
}

bool VideoSourceReader::StopStream()
{
    if (!isRunning)
        return false;
    if (streamThread.joinable())
    {
        isRunning = false;
        streamThread.join();
        streamOn = false;
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(handle, VIDIOC_STREAMOFF, &type);
    Uninitmmap();
    return true;
}

void VideoSourceReader::OnReadWorker()
{
    int index;
    while (isRunning)
    {
        auto frame = RequestFrame(index);
        if (frame)
        {
            if (fnCb || pCbobj)
            {
                std::vector<std::shared_ptr<IVideoFrame>> frames;
                frames.push_back(frame);
                //Invoke callback handler
                if (fnCb)
                    fnCb(*this, frames);
                if (pCbobj)
                    pCbobj->OnFrame(*this, frames);
                ReleaseFrame(index); //Release immediately if no callback
                //std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


//-------------------------------------------------------------------
// TryMediaType
//
// Test a proposed video format.
//-------------------------------------------------------------------

void VideoSourceReader::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	pCbobj = pCB;
}

void VideoSourceReader::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fnCb = fn;
}

//bool VideoSourceReader::IsFormatSupported(const GUID &subtype) const
//{
//	//Do we support this type directly ?
//	return true;
//}
