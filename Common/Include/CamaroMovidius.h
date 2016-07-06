#pragma once

#include "IExtensionAccess.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraSoloBase.h"
#include "ExtensionAccessAdapter.h"
#include "IMobile.h"

namespace TopGear
{
    class CamaroMovidius
        : public CameraSoloBase,
        public TopGear::ICameraControl,
        public IDeviceControl,
        public IMobile
    {
    public:
        virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc="") override;
        virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
        virtual const std::vector<VideoFormat>& GetAllFormats() const override;

        virtual const VideoFormat &GetCurrentFormat() const override;
        virtual bool SetCurrentFormat(uint32_t formatIndex) override;

        virtual bool SetControl(std::string name, IPropertyData &val) override;
        virtual bool SetControl(std::string name, IPropertyData &&val) override;
        virtual bool GetControl(std::string name, IPropertyData &val) override;

    protected:
        ExtensionAccessAdapter extensionAdapter;
        std::vector<VideoFormat> formats;
        int currentFormatIndex = -1;
    private:
        std::string sensorInfo;
        VideoFrameCallbackFn fncb = nullptr;
        std::atomic_ushort syncTag;
    public:
        virtual int Flip(bool vertical, bool horizontal) override;
        virtual int GetExposure(bool &ae, float &ev) override;
        virtual int SetExposure(bool ae, float ev=1.0f) override;
        virtual int GetShutter(uint32_t &val) override;
        virtual int SetShutter(uint32_t val) override;
        virtual int GetGain(float &gainR, float &gainG, float &gainB) override;
        virtual int SetGain(float gainR, float gainG, float gainB) override;

        virtual void StartMove() override;
        virtual void StopMove() override;
        virtual bool IsSteady() override;

        CamaroMovidius(std::shared_ptr<IVideoStream> &vs,
               std::shared_ptr<IExtensionAccess> &ex,
            CameraProfile &con = CameraProfile::NullObject());
        virtual ~CamaroMovidius();
    };
}

