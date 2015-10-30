#pragma once
#include "IGenericVCDevice.h"
#include "IExtensionUnit.h"
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

			virtual IExtensionUnit *GetExtensionUnit(bool addRef = false) const override;
			virtual const std::shared_ptr<ExtensionInfo>& GetExtensionInfo() const override;
			virtual uint32_t GetLen(int index) const override;
			static HRESULT CreateExtensionUnit(IUnknown* pUnkOuter, IExtensionUnit **ppXu);
		private:
			static const int DeviceInfoCode = 2;
			IExtensionUnit *pXu = nullptr;
			std::string deviceInfo;
			ULONG controlLens[32]{ 0 };
			std::shared_ptr<ExtensionInfo> pInfo;
			static HRESULT FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, DWORD &nodeId);
			bool ObtainInfo();
		};
	}
}

