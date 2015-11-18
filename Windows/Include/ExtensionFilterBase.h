#pragma once
#include "IGenericVCDevice.h"
#include "ExtensionInfo.h"
#include <vidcap.h>
#include "IExtensionLite.h"
#include <array>
#include "ExtensionUnit.h"

namespace TopGear
{
	namespace Win
	{
		class ExtensionFilterBase : public IExtensionLite
		{
		public:
			ExtensionFilterBase(std::shared_ptr<IGenericVCDevice> &device, const std::array<uint8_t, 16> &xucode);
			virtual ~ExtensionFilterBase();

			virtual std::shared_ptr<ExtensionUnit> GetExtensionUnit() const { return pXu; }
			
			virtual bool IsValid() const override { return pXu != nullptr; }
			virtual const std::shared_ptr<ExtensionInfo>& GetExtensionInfo() const override { return pInfo; }
			virtual uint32_t GetLen(int index) const override;
		protected:
			std::shared_ptr<ExtensionUnit> pXu;
			ULONG controlLens[32]{ 0 };
			std::shared_ptr<ExtensionInfo> pInfo;
		private:
			bool ObtainInfo();
		};
	}
}

