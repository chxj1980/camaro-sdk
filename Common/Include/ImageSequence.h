#pragma once

#include "ImageDevice.h"
#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "IProcessable.h"

#include <thread>
#include <atomic>

#include "sys_config.h"

namespace TopGear
{
    class ImageSequence
            : public IVideoStream,
              public IProcessable<std::vector<IVideoFramePtr>>
    {
    public:
        virtual bool StartStream() override;
        virtual bool StopStream() override;
        virtual bool IsStreaming() const override;

        virtual int GetOptimizedFormatIndex(VideoFormat &format, const char *fourcc = "") override;
        virtual int GetMatchedFormatIndex(const VideoFormat &format) const override;
        virtual const std::vector<VideoFormat> &GetAllFormats() const override;
        virtual const VideoFormat &GetCurrentFormat() const override;
        virtual bool SetCurrentFormat(uint32_t formatIndex) override;
        virtual void RegisterFrameCallback(const VideoFrameCallbackFn &fn) override;
        virtual void RegisterFrameCallback(IVideoFrameCallback *pCB) override;

        explicit ImageSequence(std::shared_ptr<ImageDevice> &device);
        virtual ~ImageSequence();

        virtual void Register(std::shared_ptr<IProcessor<std::vector<IVideoFramePtr>>> &processor) override
        {
            container.Register(processor);
        }
        virtual void Notify(std::shared_ptr<std::vector<IVideoFramePtr>> &payload) override
        {
            container.Notify(payload);
        }
    protected:
        std::shared_ptr<ImageDevice> imageDevice;
        std::vector<VideoFormat> formats;
        bool streaming = false;
        VideoFrameCallbackFn fncb = nullptr;
        std::thread streamThread;
    private:
        static const int BUFFER_SIZE = VIDEO_BUFFER_NUM;
        void Worker();
        std::unique_ptr<uint8_t[]> mbuffers[BUFFER_SIZE];
        std::weak_ptr<IVideoFrame> framesRef[BUFFER_SIZE];
        std::atomic_bool framesLock[BUFFER_SIZE];

        bool FindAvailableSlot(int &slot);
        void ReleaseUnusedSlot();
        ProcessorContainer<std::vector<IVideoFramePtr>> container;
    };
}

