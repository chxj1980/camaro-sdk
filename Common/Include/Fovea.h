#pragma once

#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraBase.h"
#include "BufferQueue.h"
#include "ILowlevelControl.h"

namespace TopGear
{
    class Fovea
         :  public CameraBase,
            public TopGear::ICameraControl,
            public IDeviceControl
    {
    protected:
        std::shared_ptr<TopGear::ICameraControl> wideangleCC;
        std::shared_ptr<TopGear::ICameraControl> telephotoCC;
        std::shared_ptr<IDeviceControl> wideangleDC;
        std::shared_ptr<IDeviceControl> telephotoDC;

        void FrameWatcher();
        std::thread frameWatchThread;
        bool threadOn = false;
        BufferQueue<std::pair<int, IVideoFramePtr>> frameBuffer;
        void OnWideAngleFrame(IVideoStream &wa, std::vector<IVideoFramePtr> &frames);
        void OnTelephotoFrame(IVideoStream &tp, std::vector<IVideoFramePtr> &frames);
        VideoFrameCallbackFn fnCb = nullptr;
    public:
        Fovea(std::shared_ptr<IVideoStream> &wideangle, std::shared_ptr<IVideoStream> &telephoto);
        virtual ~Fovea();
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(uint16_t& val) override;
        virtual int SetExposure(uint16_t val) override;
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
    };
}

