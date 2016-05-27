#pragma once
#include <vector>
#include "IGenericVCDevice.h"
#include "IDiscernible.h"
#include <flycapture/FlyCapture2.h>

namespace TopGear
{
    class FlyCaptureDevice
            : public IGenericVCDevice,
              public IValidation
    {
    public:
        static void EnumVideoDeviceSources(std::vector<IGenericVCDevicePtr> &inventory,
                                           std::vector<uint32_t> sns={});
        virtual std::shared_ptr<ISource>& GetSource() override { return source; }
        virtual std::string GetSymbolicLink() override { return driverName; }
        virtual std::string GetFriendlyName() override { return modelName; }
        virtual std::string GetDeviceInfo() override { return sensorInfo; }
        FlyCaptureDevice(uint32_t token, bool isSN);
        virtual ~FlyCaptureDevice() {}
        virtual bool IsValid() const override
        {
            return valid;
        }
    protected:
        std::shared_ptr<ISource> source;
        bool valid;
        std::string driverName;
        std::string modelName;
        std::string sensorInfo;
    };
}

