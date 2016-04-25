#ifndef _POINTGREY_H
#define _POINTGREY_H

#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraSoloBase.h"

namespace TopGear
{
    class PointGrey
        : public CameraSoloBase,
          public ICameraControl
    {
    public:
        virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc="") override;
        virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
        virtual const std::vector<VideoFormat>& GetAllFormats() const override;

        virtual const VideoFormat &GetCurrentFormat() const override;
        virtual bool SetCurrentFormat(uint32_t formatIndex) override;

        virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
        virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

        virtual bool StartStream() override;
        virtual bool StopStream() override;

//        virtual bool SetControl(std::string name, IPropertyData &val) override;
//        virtual bool SetControl(std::string name, IPropertyData &&val) override;
//        virtual bool GetControl(std::string name, IPropertyData &val) override;
    protected:
        void OnFrame(IVideoStream &parent, std::vector<IVideoFramePtr> &frames);
        VideoFrameCallbackFn fnCb = nullptr;
        std::vector<VideoFormat> formats;
        int currentFormatIndex = -1;
    public:
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(uint32_t& val) override;
        virtual int SetExposure(uint32_t val) override;
        virtual int GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB) override;
        virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) override;
        explicit PointGrey(std::shared_ptr<IVideoStream> &vs);
        virtual ~PointGrey();
    };
}

#endif // _POINTGREY_H

