#pragma once
#include <vector>
#include <chrono>
#include "IGenericVCDevice.h"
#include "IDeviceFactory.h"
#include "LSource.h"
#include "GenericVCDevice.h"
#include "ExtensionVCDevice.h"
#include "StandardVCDevice.h"
#include "v4l2helper.h"

namespace TopGear
{
    namespace Linux
    {
        template<class T>
        class DeviceFactory : public IDeviceFactory<T>
        {
        public:
            static std::vector<IGenericVCDevicePtr> EnumerateDevices();
        private:
            static const std::chrono::milliseconds InitialTime;
            DeviceFactory() = default;
        protected:
            ~DeviceFactory() = default;
        };

        template <>
        std::vector<IGenericVCDevicePtr> DeviceFactory<GenericVCDevice>::EnumerateDevices()
        {
            std::vector<IGenericVCDevicePtr> genericDevices;
            try
            {
                std::vector<SourcePair> inventory;
                v4l2Helper::EnumVideoDeviceSources(inventory,InitialTime);
                for (auto &sp : inventory)
                {
                    std::shared_ptr<ISource> source = std::make_shared<LSource>(sp);
                    genericDevices.emplace_back(std::make_shared<GenericVCDevice>(source));
                }
            }
            catch (const std::exception&)
            {

            }
            return genericDevices;
        }

        template<typename T>
        struct IsExtensionVCDevice
        {
            typedef T value_type;
            static constexpr bool value = false;
        };

        template<template<typename> class X, typename T>
        struct IsExtensionVCDevice<X<T>>   //specialization
        {
            typedef T value_type;
            static constexpr bool value = std::is_same<X<T>, ExtensionVCDevice<T>>::value;
        };

        template <class T>
        std::vector<IGenericVCDevicePtr> DeviceFactory<T>::EnumerateDevices()
        {
            if (!IsExtensionVCDevice<T>::value && !std::is_same<StandardVCDevice,T>::value)
                return{};
            std::vector<IGenericVCDevicePtr> result;
            auto genericDevices = DeviceFactory<GenericVCDevice>::EnumerateDevices();
            for (auto device : genericDevices)
            {
                auto exdev = std::make_shared<T>(device);
                if (exdev->GetValidator()->IsValid())
                    result.emplace_back(exdev);
            }
            return result;
        }
    }
}


