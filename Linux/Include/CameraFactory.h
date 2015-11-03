#pragma once
#include "ICameraFactory.h"
#include "IVideoStream.h"
#include "GenericVCDevice.h"
#include "ILSource.h"
#include "VideoSourceReader.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "CamaroDual.h"
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
            template<class U>
            static std::shared_ptr<IVideoStream> CreateInstance(U &u);
		private:
			CameraFactory() = default;
		protected:
			~CameraFactory() = default;
		};


        std::shared_ptr<IVideoStream> CreateCamaroDual(std::vector<IGenericVCDeviceRef> &list);

        template <class T>
        template <class U>
        std::shared_ptr<IVideoStream> CameraFactory<T>::CreateInstance(U &u)
        {
            static_assert(std::is_same<IGenericVCDeviceRef, U>::value |
                          std::is_same<std::vector<IGenericVCDeviceRef>, U>::value,
                          "Class U must be IGenericVCDeviceRef or std::vector<IGenericVCDeviceRef>");

            if (std::is_same<T,CamaroDual>::value &&
                std::is_same<std::vector<IGenericVCDeviceRef>, U>::value)
            {
                auto list = *reinterpret_cast<std::vector<IGenericVCDeviceRef> *>(&u);
                return CreateCamaroDual(list);
            }


            IGenericVCDeviceRef device;
            std::shared_ptr<ILSource> source;
            std::shared_ptr<IVideoStream> vs;
            if (std::is_same<IGenericVCDeviceRef, U>::value)
            {
                device = *reinterpret_cast<IGenericVCDeviceRef *>(&u);
                source = std::dynamic_pointer_cast<ILSource>(device);
                if (source == nullptr)
                    return {};
                vs = std::make_shared<VideoSourceReader>(source);
            }
            if (std::is_same<T,StandardUVC>::value && vs)
                return std::make_shared<StandardUVC>(vs);
            else if (std::is_same<T,Camaro>::value && device)
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
