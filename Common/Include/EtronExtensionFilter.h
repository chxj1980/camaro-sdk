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
            : ExtensionFilterBase(device, ExtensionRepository::EtronXuCode)
        {}
        virtual ~EtronExtensionFilter() {}
        virtual std::string GetDeviceInfo() override { return{}; }
    };
}
