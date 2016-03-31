#pragma once
#include <memory>
#include <array>
#include <vector>
#include "ExtensionInfo.h"
#include "IExtensionLite.h"
#include "IGenericVCDevice.h"

namespace TopGear
{
    namespace Linux
	{
        class ExtensionFilterBase : public IExtensionLite
		{
        public:
            ExtensionFilterBase(std::shared_ptr<IGenericVCDevice> &device,
                                const std::array<uint8_t, 16> &xucode,
                                std::vector<uint8_t> &&ignore={});
            virtual ~ExtensionFilterBase();

            virtual bool IsValid() const override { return isValid; }
            virtual const std::shared_ptr<ExtensionInfo>& GetExtensionInfo() const override { return pInfo; }
            virtual uint32_t GetLen(int index, bool live = false) override;
        protected:
            int controlLens[32]{ 0 };
            int handle;
            bool isValid = false;
            std::shared_ptr<ExtensionInfo> pInfo;
        private:
            bool ObtainInfo(std::vector<uint8_t> &ignore);
		};
	}
}
