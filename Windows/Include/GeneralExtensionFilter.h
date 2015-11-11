#pragma once
#include "IGenericVCDevice.h"
#include "ExtensionInfo.h"
#include <vidcap.h>
#include "IMExtensionLite.h"
#include <string>

namespace TopGear
{
	namespace Win
	{
		class GeneralExtensionFilter : public IMExtensionLite
		{
		public:
			
			explicit GeneralExtensionFilter(IUnknown *pBase);
			virtual ~GeneralExtensionFilter();

			virtual bool IsValid() const override { return pXu != nullptr; }

			virtual std::string GetDeviceInfo() override;
			virtual std::shared_ptr<ExtensionUnit> GetExtensionUnit() const override
			{
				return pXu;
			}

			virtual const std::shared_ptr<ExtensionInfo>& GetExtensionInfo() const override
			{
				return pInfo;
			}
			virtual uint32_t GetLen(int index) const override;
		private:
			static const int DeviceInfoCode = 2;
			//IExtensionUnit *pXu = nullptr;
			std::shared_ptr<ExtensionUnit> pXu;
			std::string deviceInfo;
			ULONG controlLens[32]{ 0 };
			std::shared_ptr<ExtensionInfo> pInfo;
			HRESULT CreateExtensionUnit(IUnknown* pUnkOuter);
			bool ObtainInfo();
		};

		DEFINE_GUIDSTRUCT("ffffffff-ffff-ffff-ffff-ffffffffffff", PROPSETID_VIDCAP_EXTENSION_UNIT);
		#define PROPSETID_VIDCAP_EXTENSION_UNIT DEFINE_GUIDNAMED(PROPSETID_VIDCAP_EXTENSION_UNIT)
	}
}

