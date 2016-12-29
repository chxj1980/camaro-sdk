#pragma once

#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraBase.h"
#include "BufferQueue.h"
#include "ILowlevelControl.h"
#include "IMultiVideoStream.h"
#include "IIrisControl.h"
#include "WatchDog.h"
#include "MobileChecker.h"

namespace TopGear
{
    class Fovea
         :  public CameraBase,
            public IMultiVideoStream,
            public TopGear::ICameraControl,
            public IDeviceControl,
            public IIrisControl
            public IWatch,
            public IMobile
    {
    protected:
        int currentStreamIndex = 0;
        std::shared_ptr<IVideoStream> currentStream;

        void OnWideAngleFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames);
        void OnTelephotoFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames);
        VideoFrameCallbackFn fnCb = nullptr;
        TimeoutCallbackFn tcb = nullptr;
        std::chrono::seconds interval;
        WatchDog watchdog;
    private:
        std::thread frameWatchThread;
        bool threadOn = false;
        int keyStreamIndex = 0;
        BufferQueue<std::pair<int, IVideoFramePtr>> frameBuffer;
        void FrameWatcher();
        void PushFrame(int index, IVideoFramePtr &frame);
    public:
        Fovea(std::shared_ptr<IVideoStream> &wideangle, std::shared_ptr<IVideoStream> &telephoto);
        virtual ~Fovea();
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(bool &ae, float &ev) override;
        virtual int SetExposure(bool ae, float ev=1.0f) override;
        virtual int GetShutter(uint32_t &val) override;
        virtual int SetShutter(uint32_t val) override;
        virtual int GetGain(float &gainR, float &gainG, float &gainB) override;
        virtual int SetGain(float gainR, float gainG, float gainB) override;

        virtual int GetIris(float &ratio) override;
        virtual int SetIris(float ratio) override;
        virtual int SetIrisOffset(int offset) override;

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

        virtual void RegisterTimeoutCallback(const TimeoutCallbackFn &fn, std::chrono::seconds timeout) override;

        virtual const std::vector<std::shared_ptr<IVideoStream>> &GetStreams() const override;
        virtual bool SelectStream(int index) override;
        virtual bool SelectStream(const std::shared_ptr<IVideoStream> &vs) override;
        virtual int GetCurrentStream(std::shared_ptr<IVideoStream> &current) override;
        virtual void StartStreams() override;
        virtual void StopStreams() override;

        virtual void StartMove() override;
        virtual void StopMove() override;
        virtual bool IsSteady() override;
    };
}

