#pragma once

#include <memory>
#include <type_traits>

#include "ICameraFactory.h"
#include "IVideoStream.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "CamaroISP.h"
#include "ImpalaE.h"
#include "Fovea.h"
#include "ExtensionAccess.h"
#include "VideoSourceReader.h"
#include "CamaroDual.h"
#include "ExtensionVCDevice.h"
#include "IDiscernible.h"
#include "ExtensionFilterBase.h"

#include "sys_config.h"

#ifdef SUPPORT_POINTGREY
#include "PointGrey.h"
#include "FlyCaptureReader.h"
#endif

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

        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<StandardUVC>::
            CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
        {
            auto reader = VideoSourceReader::CreateVideoStream(device);
            if (reader == nullptr)
                return{};
            return std::make_shared<StandardUVC>(reader);
        }

        bool GetUVCStream(IGenericVCDevicePtr& device,
                            std::shared_ptr<IVideoStream> &reader,
                            std::shared_ptr<IExtensionAccess> &ex)
        {
            auto exDevice = std::dynamic_pointer_cast<IDiscernible<IExtensionLite>>(device);
            if (exDevice == nullptr)
                return false;

            reader = VideoSourceReader::CreateVideoStream(device);
            if (reader == nullptr)
                return false;

            auto validator = std::dynamic_pointer_cast<ExtensionFilterBase>(exDevice->GetValidator());
            if (validator == nullptr)
                return false;
            auto lsource = std::dynamic_pointer_cast<LSource>(device->GetSource());
            if (lsource == nullptr)
                return false;
            ex = std::static_pointer_cast<IExtensionAccess>(
                        std::make_shared<ExtensionAccess>(lsource->GetHandle(), validator));
            if (ex==nullptr)
                return false;
            return true;
        }

        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<Camaro>::
        CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
        {
            std::shared_ptr<IVideoStream> reader;
            std::shared_ptr<IExtensionAccess> ex;
            if (!GetUVCStream(device,reader,ex))
                return {};
            auto it = CameraProfile::Repository.find(Camera::Camaro);
            if (it!=CameraProfile::Repository.end())
                return std::make_shared<Camaro>(reader, ex, it->second);
            return std::make_shared<Camaro>(reader, ex);
        }

        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<CamaroISP>::
        CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
        {
            std::shared_ptr<IVideoStream> reader;
            std::shared_ptr<IExtensionAccess> ex;
            if (!GetUVCStream(device,reader,ex))
                return {};
            auto it = CameraProfile::Repository.find(Camera::Camaro);
            if (it!=CameraProfile::Repository.end())
                return std::make_shared<CamaroISP>(reader, ex, it->second);
            return std::make_shared<CamaroISP>(reader, ex);
        }

#ifdef SUPPORT_POINTGREY
        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<PointGrey>::
        CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
        {
            auto vflip = std::make_shared<bool>(false);
            auto reader = FlyCaptureReader::CreateVideoStream(device, vflip);
            if (reader==nullptr)
                return {};
            auto source = std::dynamic_pointer_cast<FlyCaptureSource>(device->GetSource());
            if (source == nullptr)
                return false;
            return std::make_shared<PointGrey>(reader, device->GetSource(), vflip);
        }
#endif

        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<ImpalaE>::
        CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
        {

            auto exDevice = std::dynamic_pointer_cast<IDiscernible<IExtensionLite>>(device);
            if (exDevice == nullptr)
                return{};

            auto streams = VideoSourceReader::CreateVideoStreams(device);
            if (streams.size()==0)
                return{};

            auto validator = std::dynamic_pointer_cast<ExtensionFilterBase>(exDevice->GetValidator());
            if (validator == nullptr)
                return{};
            auto lsource = std::dynamic_pointer_cast<LSource>(device->GetSource());
            if (lsource == nullptr)
                return{};
            auto ex = std::static_pointer_cast<IExtensionAccess>(
                        std::make_shared<ExtensionAccess>(lsource->GetHandle(), validator));

            return std::make_shared<ImpalaE>(streams, ex);
        }


        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<CamaroDual>::
        CreateInstance<std::vector<IGenericVCDevicePtr>>(std::vector<IGenericVCDevicePtr> &devices)
        {
            std::shared_ptr<IVideoStream> master;
            std::shared_ptr<IVideoStream> slave;
            for (auto item : devices)
            {
                auto camera = std::dynamic_pointer_cast<Camaro>(CameraFactory<Camaro>::CreateInstance(item));
                PropertyData<uint8_t> data;
                camera->GetControl("DeviceRole", data);
                if (data.Payload == 0 && master == nullptr)
                {
                    master = std::static_pointer_cast<IVideoStream>(camera);
                }
                if (data.Payload == 1 && slave == nullptr)
                {
                    slave = std::static_pointer_cast<IVideoStream>(camera);
                }
                if (master != nullptr && slave != nullptr)
                    return std::make_shared<CamaroDual>(master, slave);
            }
            return{};
        }

        template <>
        template <>
        inline std::shared_ptr<IVideoStream> CameraFactory<Fovea>::
        CreateInstance<std::vector<std::shared_ptr<IVideoStream>>>(std::vector<std::shared_ptr<IVideoStream>> &vss)
        {
            if (vss.size()!=2)
                return {};
            return std::make_shared<Fovea>(vss[0], vss[1]);
        }
        

        template<class T>
        template<class U>
        inline std::shared_ptr<IVideoStream> CameraFactory<T>::CreateInstance(U& device)
        {
            (void)device;
            throw std::invalid_argument("Unimplementation");
            //return{};
        }
	}
}
