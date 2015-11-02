#include "GenericVCDevice.h"
#include "GeneralExtensionFilter.h"
#include "StandardUVCFilter.h"
#include "MFHelper.h"
#include "DeviceFactory.h"

using namespace TopGear;
using namespace Win;

template<class T>
const std::chrono::milliseconds DeviceFactory<T>::InitialTime = std::chrono::milliseconds(100);

template <>
std::vector<IGenericVCDeviceRef> DeviceFactory<GenericVCDevice>::EnumerateDevices()
{
	std::vector<IGenericVCDeviceRef> devices;
	try
	{
		std::vector<SourcePair> inventory;
		MFHelper::EnumVideoDeviceSources(inventory, InitialTime);
		for (auto source : inventory)
		{
			IGenericVCDeviceRef pDevice = std::make_shared<GenericVCDevice>(source.first, source.second);
			devices.push_back(pDevice);
		}
	}
	catch (const std::exception&)
	{

	}
	return devices;
}

template <>
std::vector<IGenericVCDeviceRef> DeviceFactory<StandardVCDevice>::EnumerateDevices()
{
	std::vector<IGenericVCDeviceRef> result;
	auto genericDevices = DeviceFactory<GenericVCDevice>::EnumerateDevices();
	for (auto device : genericDevices)
	{
		auto gDevice = std::dynamic_pointer_cast<IMSource>(device);
		if (gDevice == nullptr)
			continue;
		auto validator = std::make_shared<StandardUVCFilter>(gDevice->GetSource());
		if (validator->IsValid())
			result.push_back(device);
	}
	return result;
}

template <>
std::vector<IGenericVCDeviceRef> DeviceFactory<DiscernibleVCDevice>::EnumerateDevices()
{
	std::vector<IGenericVCDeviceRef> result;
	auto genericDevices = DeviceFactory<GenericVCDevice>::EnumerateDevices();
	for (auto device : genericDevices)
	{
		auto gDevice = std::dynamic_pointer_cast<IMSource>(device);
		if (gDevice == nullptr)
			continue;
		std::shared_ptr<IMExtensionLite> validator = std::make_shared<GeneralExtensionFilter>(gDevice->GetSource());
		if (validator->IsValid())
			result.push_back(std::make_shared<DiscernibleVCDevice>(device, validator));
	}
	return result;
}

template <class T>
std::vector<IGenericVCDeviceRef> DeviceFactory<T>::EnumerateDevices()
{
	return{};
}