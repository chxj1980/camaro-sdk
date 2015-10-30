#pragma once
#include "IVideoStream.h"
#include <vector>
#include "IGenericVCDevice.h"
#include "CameraComboBase.h"

namespace TopGear
{
	template<class T>
	class ICameraFactory
	{
		static_assert(std::is_base_of<IVideoStream, T>::value,
			"Class T must derive from IGenericVCDevice");
	public:
		static std::shared_ptr<IVideoStream> CreateInstance(IGenericVCDeviceRef &device) = delete;
	protected:
		ICameraFactory() = default;
		~ICameraFactory() = default;
	};

	template<class T>
	class ICameraComboFactory
	{
		static_assert(std::is_base_of<CameraComboBase, T>::value,
			"Class T must derive from CameraComboBase");
	public:
		static std::shared_ptr<IVideoStream> CreateInstance(std::vector<IGenericVCDeviceRef> &devices) = delete;
	protected:
		ICameraComboFactory() = default;
		~ICameraComboFactory() = default;
	};
}
