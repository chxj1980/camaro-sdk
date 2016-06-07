#ifndef _POINTGREY_H
#define _POINTGREY_H

#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraSoloBase.h"
#include "FlyCaptureSource.h"

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

//        virtual bool SetControl(std::string name, IPropertyData &val) override;
//        virtual bool SetControl(std::string name, IPropertyData &&val) override;
//        virtual bool GetControl(std::string name, IPropertyData &val) override;
    protected:
        std::vector<VideoFormat> formats;
        int currentFormatIndex = -1;
        std::shared_ptr<FlyCaptureSource> pgsource;
        std::shared_ptr<bool> flipState;
    public:
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(bool &ae, float &ev) override;
        virtual int SetExposure(bool ae, float ev=1.0f) override;
        virtual int GetShutter(uint32_t &val) override;
        virtual int SetShutter(uint32_t val) override;
        virtual int GetGain(float &gainR, float &gainG, float &gainB) override;
        virtual int SetGain(float gainR, float gainG, float gainB) override;
        PointGrey(std::shared_ptr<IVideoStream> &vs, std::shared_ptr<ISource> &source,
                  std::shared_ptr<bool> &vflip);
        virtual ~PointGrey();
    };
}

#endif // _POINTGREY_H

