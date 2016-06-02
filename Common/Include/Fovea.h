#pragma once

#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraBase.h"
#include "BufferQueue.h"
#include "ILowlevelControl.h"
#include "IMultiVideoStream.h"

namespace TopGear
{
    class Fovea
         :  public CameraBase,
            public IMultiVideoStream,
            public TopGear::ICameraControl,
            public IDeviceControl
    {
    protected:
        int currentStreamIndex = 0;
        std::shared_ptr<IVideoStream> currentStream;

        void OnWideAngleFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames);
        void OnTelephotoFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames);
        VideoFrameCallbackFn fnCb = nullptr;
    private:
        std::thread frameWatchThread;
        bool threadOn = false;
        int keyStreamIndex = -1;
        BufferQueue<std::pair<int, IVideoFramePtr>> frameBuffer;
        void FrameWatcher();
        void PushFrame(int index, IVideoFramePtr &frame);
    public:
        Fovea(std::shared_ptr<IVideoStream> &wideangle, std::shared_ptr<IVideoStream> &telephoto);
        virtual ~Fovea();
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(uint32_t& val) override;
        virtual int SetExposure(uint32_t val) override;
        virtual int GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB) override;
        virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) override;

        virtual bool SetControl(std::string name, IPropertyData &val) override;
        virtual bool SetControl(std::string name, IPropertyData &&val) override;
        virtual bool GetControl(std::string name, IPropertyData &val) override;

        virtual bool StartStream() override;
        virtual bool StopStream() override;
        virtual bool IsStreaming() const override;

        virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc = "") override;
        virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
        virtual const std::vector<VideoFormat>& GetAllFormats() const override;
        virtual const VideoFormat &GetCurrentFormat() const override;
        virtual bool SetCurrentFormat(uint32_t formatIndex) override;
        virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
        virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

        virtual const std::vector<std::shared_ptr<IVideoStream>> &GetStreams() const override;
        virtual bool SelectStream(int index) override;
        virtual bool SelectStream(const std::shared_ptr<IVideoStream> &vs) override;
        virtual int GetCurrentStream(std::shared_ptr<IVideoStream> &current) override;
        virtual void StartStreams() override;
        virtual void StopStreams() override;
    };
}

