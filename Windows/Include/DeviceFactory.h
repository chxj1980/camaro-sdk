#pragma once
#include <vector>
#include <chrono>
#include "IGenericVCDevice.h"
#include "IDeviceFactory.h"
#include "ExtensionVCDevice.h"
#include "GenericVCDevice.h"
#include "StandardVCDevice.h"
#include "MFHelper.h"

namespace TopGear
{
	namespace Win
	{
		template<typename T>
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
		inline std::vector<IGenericVCDevicePtr> DeviceFactory<GenericVCDevice>::EnumerateDevices()
		{
			std::vector<IGenericVCDevicePtr> devices;
			try
			{
				std::vector<SourcePair> inventory;
				MFHelper::EnumVideoDeviceSources(inventory, InitialTime);
				for (auto source : inventory)
				{
					IGenericVCDevicePtr pDevice = std::make_shared<GenericVCDevice>(source.first, source.second);
					devices.push_back(pDevice);
				}
			}
			catch (const std::exception&)
			{

			}
			return devices;
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

		//template <>
		//std::vector<IGenericVCDeviceRef> DeviceFactory<StandardVCDevice>::EnumerateDevices()
		//{
		//	std::vector<IGenericVCDeviceRef> result;
		//	auto genericDevices = DeviceFactory<GenericVCDevice>::EnumerateDevices();
		//	for (auto device : genericDevices)
		//	{
		//		auto gDevice = std::dynamic_pointer_cast<IMSource>(device);
		//		if (gDevice == nullptr)
		//			continue;
		//		auto validator = std::make_shared<StandardUVCFilter>(gDevice->GetSource());
		//		if (validator->IsValid())
		//			result.push_back(device);
		//	}
		//	return result;
		//}
	}
}


