#pragma once
#include "ExtensionFilterBase.h"

namespace TopGear
{
    namespace Linux
    {
        class DGExtensionFilter : public ExtensionFilterBase
        {
        public:
            explicit DGExtensionFilter(std::shared_ptr<IGenericVCDevice> &device);
            virtual ~DGExtensionFilter() {}
            virtual std::string GetDeviceInfo() override;
        private:
            static const int DeviceInfoCode = 2;
            std::string deviceInfo;
        };
    }
}

