#include "DeepCamAPI.h"
#include "DeviceFactory.h"
#include "GenericVCDevice.h"
#include "DGExtensionFilter.h"
#include "EtronExtensionFilter.h"
#include "System.h"
#include "CameraFactory.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "CamaroDual.h"
#include "IMultiVideoStream.h"

using namespace TopGear;
using namespace Win;

class DeepCamAPIInternal
{
	friend class DeepCamAPI;
	static std::unique_ptr<DeepCamAPI> Instance;
	
	template<class U>
	static std::shared_ptr<IVideoStream> CreateCamera(Camera camera, U & source)
	{
		static_assert(std::is_same<IGenericVCDeviceRef, U>::value || std::is_same<std::vector<IGenericVCDeviceRef>, U>::value,
			"Parameter source must be type of IGenericVCDeviceRef or std::vector<IGenericVCDeviceRef>");
		std::shared_ptr<IVideoStream> vs;
		switch (camera)
		{
		case Camera::StandardUVC:
			vs = CameraFactory<StandardUVC>::CreateInstance(source);
			break;
		case Camera::Camaro:
			vs = CameraFactory<Camaro>::CreateInstance(source);
			break;
		case Camera::CamaroDual:
			vs = CameraFactory<CamaroDual>::CreateInstance(source);
			break;
		case Camera::Etron3D:
			break;
		default:
			break;
		}
		return vs;
	}
};

std::unique_ptr<DeepCamAPI> DeepCamAPIInternal::Instance;

DeepCamAPI &DeepCamAPI::Instance()
{
	if (DeepCamAPIInternal::Instance == nullptr)
		DeepCamAPIInternal::Instance.reset(new DeepCamAPI);
	return *DeepCamAPIInternal::Instance;
}

DeepCamAPI::DeepCamAPI()
{
	System::Initialize();
}

DeepCamAPI::~DeepCamAPI()
{
	System::Dispose();
}

std::vector<IGenericVCDeviceRef> DeepCamAPI::EnumerateDevices(DeviceType type) const
{
	std::vector<IGenericVCDeviceRef> inventory;
	switch (type)
	{
	case DeviceType::Generic:
		inventory = DeviceFactory<GenericVCDevice>::EnumerateDevices();
		break;
	case DeviceType::Standard: 
		inventory = DeviceFactory<StandardVCDevice>::EnumerateDevices();
		break;
	case DeviceType::DeepGlint: 
		inventory = DeviceFactory<ExtensionVCDevice<DGExtensionFilter>>::EnumerateDevices();
		break;
	case DeviceType::Etron: 
		inventory = DeviceFactory<ExtensionVCDevice<EtronExtensionFilter>>::EnumerateDevices();
		break;
	default: 
		break;
	}
	return inventory;
}

template<>
std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<IGenericVCDeviceRef>(Camera camera, IGenericVCDeviceRef & source)
{
	return DeepCamAPIInternal::CreateCamera(camera, source);
}

template<>
std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<std::vector<IGenericVCDeviceRef>>(Camera camera, std::vector<IGenericVCDeviceRef> & source)
{
	return DeepCamAPIInternal::CreateCamera(camera, source);
}

template<class U>
std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera(Camera camera, U & source)
{
	return DeepCamAPIInternal::CreateCamera(camera, source);
}

template<>
std::shared_ptr<TopGear::ICameraControl> DeepCamAPI::QueryInterface<TopGear::ICameraControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<TopGear::ICameraControl>(vs);
}

template<>
std::shared_ptr<IDeviceControl> DeepCamAPI::QueryInterface<IDeviceControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<IDeviceControl>(vs);
}

template<>
std::shared_ptr<ILowlevelControl> DeepCamAPI::QueryInterface<ILowlevelControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<ILowlevelControl>(vs);
}

template<>
std::shared_ptr<IMultiVideoStream> DeepCamAPI::QueryInterface<IMultiVideoStream>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<IMultiVideoStream>(vs);
}
