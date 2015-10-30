#pragma once
#include "ICameraFactory.h"
#include "IVideoStream.h"
#include "GenericVCDevice.h"
#include "ILSource.h"
#include "VideoSourceReader.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "ExtensionAccess.h"
#include <memory>
#include <type_traits>

namespace TopGear
{
    namespace Linux
	{
		template<class T>
		class CameraFactory : public ICameraFactory<T> 
		{
		public:
			static std::shared_ptr<IVideoStream> CreateInstance(IGenericVCDeviceRef &device);
		private:
			CameraFactory() = default;
		protected:
			~CameraFactory() = default;
		};

		template<class T>
		class CameraComboFactory : public ICameraComboFactory<T>
		{
		public:
			static std::shared_ptr<IVideoStream> CreateInstance(std::vector<IGenericVCDeviceRef> &devices);
		private:
			CameraComboFactory() = default;
		protected:
			~CameraComboFactory() = default;
		};

        template <class T>
        std::shared_ptr<IVideoStream> CameraFactory<T>::CreateInstance(IGenericVCDeviceRef& device)
        {
            auto source = std::dynamic_pointer_cast<ILSource>(device);
            if (source == nullptr)
                return {};
            std::shared_ptr<IVideoStream> vs(new VideoSourceReader(source));
            if (std::is_same<T,StandardUVC>::value)
                return std::make_shared<StandardUVC>(vs);
            else if (std::is_same<T,Camaro>::value)
            {
                auto tgDevice = std::dynamic_pointer_cast<ITopGearGeneralDevice>(device);
                if (tgDevice == nullptr)
                    return {};
                auto validator = tgDevice->GetValidator();
                std::shared_ptr<IExtensionAccess> ex(
                            new ExtensionAccess(source->GetSource(),validator));
                return std::make_shared<Camaro>(vs, ex);
            }
            return {};
        }
	}
}
