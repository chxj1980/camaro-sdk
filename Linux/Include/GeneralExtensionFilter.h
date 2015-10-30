#pragma once
#include "IGenericVCDevice.h"
#include "ExtensionInfo.h"
#include "ILExtensionLite.h"
#include <string>

namespace TopGear
{
    namespace Linux
	{
        class GeneralExtensionFilter : public ILExtensionLite
		{
		public:
            explicit GeneralExtensionFilter(int dev);
            GeneralExtensionFilter(int dev,std::shared_ptr<ExtensionInfo> &xu);
			virtual ~GeneralExtensionFilter();

            virtual bool IsValid() const override { return unitId>-1; }

			virtual std::string GetDeviceInfo() override;

            virtual int GetExtensionUnit() const override { return unitId; }
			virtual const std::shared_ptr<ExtensionInfo>& GetExtensionInfo() const override;
			virtual uint32_t GetLen(int index) const override;
		private:
            static const uint8_t guidCode[16];
			static const int DeviceInfoCode = 2;
            int handle;
            int unitId;
			std::string deviceInfo;
            int controlLens[32]{ 0 };
			std::shared_ptr<ExtensionInfo> pInfo;
			bool ObtainInfo();
		};
	}
}

