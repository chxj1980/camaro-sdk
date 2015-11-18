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
#include "StandardVCDevice.h"

using namespace TopGear;
using namespace Win;

bool System::inited = false;

template<typename T>
const std::chrono::milliseconds DeviceFactory<T>::InitialTime = std::chrono::milliseconds(100);

enum class DeviceType
{
	Generic,
	Standard,
	DeepGlint,
	Etron,
};

class DeepCamAPIInternal
{
	friend class DeepCamAPI;
	static std::unique_ptr<DeepCamAPI> Instance;

	static std::vector<IGenericVCDevicePtr> EnumerateDevices(DeviceType type)
	{
		std::vector<IGenericVCDevicePtr> inventory;
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
	
	template<class U>
	static std::shared_ptr<IVideoStream> CreateCamera(Camera camera, U & source)
	{
		static_assert(std::is_same<IGenericVCDevicePtr, U>::value || std::is_same<std::vector<IGenericVCDevicePtr>, U>::value,
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
		DeepCamAPIInternal::Instance = std::unique_ptr<DeepCamAPI>(new DeepCamAPI);
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



//template<>
//DEEPCAM_API std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<IGenericVCDevicePtr>(Camera camera, IGenericVCDevicePtr & source)
//{
//	return DeepCamAPIInternal::CreateCamera(camera, source);
//}
//
//template<>
//DEEPCAM_API std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<std::vector<IGenericVCDevicePtr>>(Camera camera, std::vector<IGenericVCDevicePtr> & source)
//{
//	return DeepCamAPIInternal::CreateCamera(camera, source);
//}

template<>
DEEPCAM_API std::shared_ptr<TopGear::ICameraControl> DeepCamAPI::QueryInterface<TopGear::ICameraControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<TopGear::ICameraControl>(vs);
}

template<>
DEEPCAM_API std::shared_ptr<IDeviceControl> DeepCamAPI::QueryInterface<IDeviceControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<IDeviceControl>(vs);
}

template<>
DEEPCAM_API std::shared_ptr<ILowlevelControl> DeepCamAPI::QueryInterface<ILowlevelControl>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<ILowlevelControl>(vs);
}

template<>
DEEPCAM_API std::shared_ptr<IMultiVideoStream> DeepCamAPI::QueryInterface<IMultiVideoStream>(std::shared_ptr<IVideoStream> &vs) const
{
	return std::dynamic_pointer_cast<IMultiVideoStream>(vs);
}

DEEPCAM_API std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera(Camera camera)
{
	std::vector<IGenericVCDevicePtr> inventory;
	std::shared_ptr<IVideoStream> vs;
	switch (camera)
	{
	case Camera::StandardUVC:
		inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::Standard);
		if (inventory.size()>0)
			vs = CameraFactory<StandardUVC>::CreateInstance(inventory[0]);
		break;
	case Camera::Camaro:
		inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::DeepGlint);
		for (auto device : inventory)
		{
			vs = CameraFactory<Camaro>::CreateInstance(device);
			auto dc = std::dynamic_pointer_cast<IDeviceControl>(vs);
			if (dc && dc->QueryDeviceRole() == 0)
				break;
			else
				vs.reset();
		}
		break;
	case Camera::CamaroDual:
		inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::DeepGlint);
		vs = CameraFactory<CamaroDual>::CreateInstance(inventory);
		break;
	case Camera::Etron3D:
		break;
	default:
		break;
	}
	return vs;
}
