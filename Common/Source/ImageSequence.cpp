#include "ImageSequence.h"
#include "VideoBufferLock.h"

#include <fstream>
#include <chrono>
#include <functional>

#include "turbojpeg.h"

using namespace TopGear;
using namespace Linux;

ImageSequence::ImageSequence(std::shared_ptr<ImageDevice> &device)
    : imageDevice(device)
{
    auto &format = device->GetFormat();
    formats.push_back(format);
    std::memcpy(formats[0].PixelFormat, "I420", 4);

    //Clear buffers
    for(auto i = 0; i < BUFFER_SIZE; ++i)
    {
        mbuffers[i] = std::unique_ptr<uint8_t[]>(new uint8_t[format.Height*format.Width*3/2]);
        framesLock[i] = false;
    }
}

ImageSequence::~ImageSequence()
{
    StopStream();
}

int ImageSequence::GetOptimizedFormatIndex(VideoFormat &format, const char *fourcc)
{
    format = formats[0];
    (void)fourcc;
    return 0;
}

int ImageSequence::GetMatchedFormatIndex(const VideoFormat &format) const
{
    (void)format;
    return 0;
}

const std::vector<VideoFormat> &ImageSequence::GetAllFormats() const
{
    return formats;
}

const VideoFormat &ImageSequence::GetCurrentFormat() const
{
    return formats[0];
}

bool ImageSequence::SetCurrentFormat(uint32_t formatIndex)
{
    return (formatIndex==0);
}

bool ImageSequence::StartStream()
{
    if (streaming)
        StopStream();
    streaming = true;
    streamThread = std::thread(&ImageSequence::Worker, this);
    return true;
}

bool ImageSequence::StopStream()
{
    if (!streaming)
        return false;
    streaming = false;
    if (streamThread.joinable())
        streamThread.join();
    return true;
}

bool ImageSequence::IsStreaming() const
{
    return streaming;
}

void ImageSequence::RegisterFrameCallback(const VideoFrameCallbackFn &fn)
{
    fncb = fn;
}

void ImageSequence::RegisterFrameCallback(IVideoFrameCallback *pCB)
{
    fncb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

bool ImageSequence::FindAvailableSlot(int &slot)
{
    slot = 0;
    while(slot<BUFFER_SIZE)
    {
        if (!framesLock[slot])
            break;
        ++slot;
    }
    return slot<BUFFER_SIZE;
}

void ImageSequence::ReleaseUnusedSlot()
{
    for(auto i=0;i<BUFFER_SIZE;++i)
        if (framesLock[i] & framesRef[i].expired())
            framesLock[i] = false;
}

void ImageSequence::Worker()
{
    auto source = std::dynamic_pointer_cast<FileSource>(imageDevice->GetSource());
    if (source==nullptr)
    {
        streaming = false;
        return;
    }
    auto &list = source->GetList();
    if (list.size()==0)
    {
        streaming = false;
        return;
    }

    tjhandle handle = tjInitDecompress();

    int index = 0;
    auto it = list.begin();

    std::chrono::milliseconds period(1000/formats[0].MaxRate);

    auto lastTime = std::chrono::high_resolution_clock::now();
    uint64_t counter = 0;
    while (streaming)
    {
        ReleaseUnusedSlot();

        auto nowTime = std::chrono::high_resolution_clock::now();

        if (nowTime-lastTime<period)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        lastTime = nowTime;

        std::ifstream ifs(*it, std::ifstream::binary);
        if (ifs)
        {
            // get length of file:
            ifs.seekg (0, ifs.end);
            int length = ifs.tellg();
            ifs.seekg (0, ifs.beg);
            std::unique_ptr<uint8_t[]> buffer(new uint8_t[length]);
            ifs.read(reinterpret_cast<char*>(buffer.get()), length);

            int w, h;
            int subsample;
            auto suc = tjDecompressHeader2(handle, buffer.get(), length, &w, &h, &subsample);
            if (suc==0)
            {
                auto size = tjBufSizeYUV(w, h, subsample);
                //Find available buffer slot
                int slot;
                if (FindAvailableSlot(slot))
                {
                    suc = tjDecompressToYUV(handle, buffer.get(), length, mbuffers[slot].get(), 0);
                    if (suc==0)
                    {
                        auto tm = std::chrono::duration_cast<std::chrono::microseconds>(
                                    std::chrono::high_resolution_clock::now().time_since_epoch());

                        std::shared_ptr<IVideoFrame> frame(new VideoBufferLock(
                                                               0,
                                                               index,
                                                               mbuffers[slot].get(),
                                                               tm.count(),
                                                               counter++,
                                                               w,
                                                               formats[0],
                                                               size));
                        framesRef[slot] = frame;
                        framesLock[slot] = true;
                        auto vector = std::shared_ptr<std::vector<IVideoFramePtr>>(new std::vector<IVideoFramePtr> { frame } );
                        Notify(vector);
                        if (fncb)
                            fncb(*this, *vector);
                    }
                }
            }
            ifs.close();
        }

        if (++it == list.end())
        {
            it = list.begin();
            index = 0;
        }
        else
            ++index;
    }

    tjDestroy(handle);
}
