#pragma once
#include <vector>
#include "IGenericVCDevice.h"
#include "ILSource.h"
#include "ILExtensionLite.h"

namespace TopGear
{
    namespace Linux
	{
        class GenericVCDevice : public IGenericVCDevice, public ILSource
		{
		public:
            virtual int GetSource() override { return handle; }
            virtual std::string GetSymbolicLink() override { return symbol; }
			virtual std::string GetFriendlyName() override;
            virtual std::string GetDeviceInfo() override { return {}; }
            GenericVCDevice(std::string dev, int fd);
			virtual ~GenericVCDevice();
		protected:
            std::string symbol;
            int handle;
			std::string name;
		};

		class StandardVCDevice final : GenericVCDevice
		{
            StandardVCDevice(std::string dev, int fd)
                : GenericVCDevice(dev, fd)
			{
			}
			virtual ~StandardVCDevice() override {}
		};

        typedef IDiscernibleVCDevice<ILExtensionLite> ITopGearGeneralDevice;

        class DiscernibleVCDevice : public IDiscernibleVCDevice<ILExtensionLite>, public ILSource
        {
        public:
            virtual int GetSource() override
            {
                return source->GetSource();
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

            virtual const std::shared_ptr<ILExtensionLite>& GetValidator() const override
            {
                return validator;
            }

            DiscernibleVCDevice(std::shared_ptr<IGenericVCDevice> &device, std::shared_ptr<ILExtensionLite> &lite)
                : genericDevice(device), validator(lite)
            {
                source = std::dynamic_pointer_cast<ILSource>(genericDevice);
            }
            virtual ~DiscernibleVCDevice() {}
        protected:
            std::shared_ptr<IGenericVCDevice> genericDevice;
            std::shared_ptr<ILSource> source;
            std::shared_ptr<ILExtensionLite> validator;
        };
	}
}
