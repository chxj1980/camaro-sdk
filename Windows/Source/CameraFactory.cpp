#include "CameraFactory.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "GenericVCDevice.h"
#include "ExtensionAccess.h"
//#include "System.h"
#include "VideoSourceReader.h"
#include "CamaroDual.h"
#include "ExtensionVCDevice.h"

using namespace TopGear;
using namespace Win;

template <>
template <>
std::shared_ptr<IVideoStream> CameraFactory<StandardUVC>::
	CreateInstance<IGenericVCDeviceRef>(IGenericVCDeviceRef& device)
{
	auto source = std::dynamic_pointer_cast<ISource>(device);
	if (source == nullptr)
		return {};
	auto list = VideoSourceReader::CreateInstances(source);
	if (list.size() == 0)
		return{};
	return std::make_shared<StandardUVC>(list[0]);
}

template <>
template <>
std::shared_ptr<IVideoStream> CameraFactory<Camaro>::
	CreateInstance<IGenericVCDeviceRef>(IGenericVCDeviceRef& device)
{
	auto tgDevice = std::dynamic_pointer_cast<IExtensionDevice>(device);
	if (tgDevice == nullptr)
		return {};

	auto source = std::dynamic_pointer_cast<ISource>(device);
	if (source == nullptr)
		return{};
	auto list = VideoSourceReader::CreateInstances(source);
	if (list.size() == 0)
		return{};

	auto validator = tgDevice->GetValidator();
	auto ex = std::static_pointer_cast<IExtensionAccess>(
		std::make_shared<ExtensionAccess>(validator));
	
	return std::make_shared<Camaro>(list[0], ex);
}


template <>
template <>
std::shared_ptr<IVideoStream> CameraFactory<CamaroDual>::
	CreateInstance<std::vector<IGenericVCDeviceRef>>(std::vector<IGenericVCDeviceRef> &devices)
{
	std::shared_ptr<IVideoStream> master;
	std::shared_ptr<IVideoStream> slave;
	for(auto item : devices)
	{
		auto camera = std::dynamic_pointer_cast<Camaro>(CameraFactory<Camaro>::CreateInstance(item));
		if (camera->QueryDeviceRole() == 0 && master == nullptr)
		{
			master = std::static_pointer_cast<IVideoStream>(camera);
		}
		if (camera->QueryDeviceRole() == 1 && slave == nullptr)
		{
			slave = std::static_pointer_cast<IVideoStream>(camera);
		}
		if (master!=nullptr && slave!=nullptr)
			return std::make_shared<CamaroDual>(master, slave);
	}
	return {};
}

template<class T>
template<class U>
std::shared_ptr<IVideoStream> CameraFactory<T>::CreateInstance(U& device)
{
	throw std::exception("Unimplementation");
	//return{};
}