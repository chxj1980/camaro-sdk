#pragma once

#include "ExtensionFilterBase.h"
#include "ExtensionRepository.h"

namespace TopGear
{
    class EtronExtensionFilter
#ifdef __linux__
                    : public Linux::ExtensionFilterBase
#elif defined(_WIN32)
                    : public Win::ExtensionFilterBase
#endif
    {
    public:
        explicit EtronExtensionFilter(std::shared_ptr<IGenericVCDevice> &device)
#ifdef __linux__
                    : ExtensionFilterBase(device, ExtensionRepository::EtronXuCode, std::vector<uint8_t> {0x0c})
#elif defined(_WIN32)
                    : ExtensionFilterBase(device, ExtensionRepository::EtronXuCode)
#endif

        {}
        virtual ~EtronExtensionFilter() {}
        virtual std::string GetDeviceInfo() override { return{}; }
    };
}
