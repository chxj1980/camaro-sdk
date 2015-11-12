#pragma once
#include <vector>
#include <chrono>
#include "IGenericVCDevice.h"
#include "IDeviceFactory.h"
#include "ExtensionVCDevice.h"

namespace TopGear
{
	namespace Win
	{
		template<typename T>
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

		template <>
		std::vector<IGenericVCDeviceRef> DeviceFactory<GenericVCDevice>::EnumerateDevices();
		template <>
		std::vector<IGenericVCDeviceRef> DeviceFactory<StandardVCDevice>::EnumerateDevices();


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
		std::vector<IGenericVCDeviceRef> DeviceFactory<T>::EnumerateDevices()
		{
			if (!IsExtensionVCDevice<T>::value)
				return{};
			std::vector<IGenericVCDeviceRef> result;
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


