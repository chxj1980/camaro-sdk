#pragma once
#include <vector>
#include <chrono>
#include "IGenericVCDevice.h"
#include "IDeviceFactory.h"
#include "ILSource.h"
#include "GenericVCDevice.h"
#include "StandardUVCFilter.h"
#include "GeneralExtensionFilter.h"
#include "v4l2helper.h"

namespace TopGear
{
    namespace Linux
    {
        template<class T>
        class DeviceFactory : public IDeviceFactory<T>
        {
        public:
            static std::vector<IGenericVCDeviceRef> EnumerateDevices();
        private:
            static const std::chrono::milliseconds InitialTime;
            DeviceFactory() = default;
        protected:
            ~DeviceFactory() = default;
        };

        template<class T>
        const std::chrono::milliseconds DeviceFactory<T>::InitialTime = std::chrono::milliseconds(100);

        template <class T>
        std::vector<IGenericVCDeviceRef> DeviceFactory<T>::EnumerateDevices()
        {
            std::vector<IGenericVCDeviceRef> genericDevices;
            try
            {
                std::vector<SourcePair> inventory;
                v4l2Helper::EnumVideoDeviceSources(inventory,InitialTime);
                for (auto source : inventory)
                {
                    IGenericVCDeviceRef pDevice(new GenericVCDevice(source.first, source.second));
                    genericDevices.push_back(pDevice);
                }
            }
            catch (const std::exception&)
            {

            }
            if (std::is_same<T,GenericVCDevice>::value)
                return genericDevices;

            std::vector<IGenericVCDeviceRef> result;
            for (auto device : genericDevices)
            {
                auto gDevice = std::dynamic_pointer_cast<ILSource>(device);
                if (gDevice == nullptr)
                    continue;
                if (std::is_same<T,StandardVCDevice>::value)
                {
                    std::shared_ptr<IValidation> validator = std::make_shared<StandardUVCFilter>(gDevice->GetSource());
                    if (validator->IsValid())
                        result.push_back(device);
                }
                else if (std::is_same<T,DiscernibleVCDevice>::value)
                {
                    std::shared_ptr<ILExtensionLite> validator = std::make_shared<GeneralExtensionFilter>(gDevice->GetSource());
                    if (validator->IsValid())
                        result.push_back(std::make_shared<DiscernibleVCDevice>(device, validator));
                }
            }
            return result;
        }
    }
}


