#pragma once
#include <vector>
#include <mfidl.h>
#include "IGenericVCDevice.h"
#include "IMExtensionLite.h"
#include "IMSource.h"

namespace TopGear
{
	namespace Win
	{
		class GenericVCDevice : public IGenericVCDevice, public IMSource
		{
		public:
			//static std::vector<IGenericVCDeviceRef> EnumerateDevices();
			virtual IMFActivate* GetActivator() override { return pActivate; }
			virtual IMFMediaSource* GetSource() override { return pSource; }
			virtual std::string GetSymbolicLink() override;
			virtual std::string GetFriendlyName() override;
			virtual std::string GetDeviceInfo() override { return ""; }
			GenericVCDevice(IMFActivate *pAct, IMFMediaSource *pSrc);
			virtual ~GenericVCDevice();
		private:
			bool GetAllocatedString(const GUID& guidCode, std::string &val) const;
		protected:
			IMFActivate *pActivate;
			IMFMediaSource *pSource;
			std::string symbolicLink;
			std::string name;
		};

		class StandardVCDevice final : GenericVCDevice
		{
			StandardVCDevice(IMFActivate* pAct, IMFMediaSource* pSrc)
				: GenericVCDevice(pAct, pSrc)
			{
			}
			virtual ~StandardVCDevice() override {}
		};

		typedef IDiscernibleVCDevice<IMExtensionLite> ITopGearGeneralDevice;

		class DiscernibleVCDevice : public IDiscernibleVCDevice<IMExtensionLite>, public IMSource
		{
		public:
			virtual IMFActivate* GetActivator() override
			{
				return source? source->GetActivator() : nullptr;
			}
			virtual IMFMediaSource* GetSource() override
			{
				return source ? source->GetSource() : nullptr;
			}
			virtual std::string GetSymbolicLink() override
			{
				return genericDevice->GetSymbolicLink();
			}
			virtual std::string GetFriendlyName() override
			{
				return genericDevice->GetFriendlyName();
			}
			virtual std::string GetDeviceInfo() override
			{
				return validator->GetDeviceInfo();
			}

			virtual const std::shared_ptr<IMExtensionLite>& GetValidator() const override
			{
				return validator;
			}

			DiscernibleVCDevice(std::shared_ptr<IGenericVCDevice> &device, std::shared_ptr<IMExtensionLite> &lite)
				: genericDevice(device), validator(lite)
			{
				source = std::dynamic_pointer_cast<IMSource>(genericDevice);
			}
			virtual ~DiscernibleVCDevice() {}
		protected:
			std::shared_ptr<IGenericVCDevice> genericDevice;
			std::shared_ptr<IMSource> source;
			std::shared_ptr<IMExtensionLite> validator;
		};
	}
}
