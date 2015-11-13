#pragma once
#include "IVideoStream.h"
#include <vector>
//#include "IGenericVCDevice.h"
//#include "CameraBase.h"

namespace TopGear
{
	template<class T>
	class ICameraFactory
	{
		static_assert(std::is_base_of<IVideoStream, T>::value,
			"Class T must derive from IGenericVCDevice");
	public:
		template<class U>
		static std::shared_ptr<IVideoStream> CreateInstance(U &device) = delete;
		~ICameraFactory() = default;
	protected:
		ICameraFactory() = default;
		
	};
}
