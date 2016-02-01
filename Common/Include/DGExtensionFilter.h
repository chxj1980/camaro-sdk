#pragma once
#include "ExtensionFilterBase.h"

namespace TopGear
{
    class DGExtensionFilter
#ifdef __linux__
            : public Linux::ExtensionFilterBase
#elif defined(_WIN32)
            : public Win::ExtensionFilterBase
#endif
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

