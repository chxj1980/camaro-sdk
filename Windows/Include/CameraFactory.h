#pragma once

#include "ICameraFactory.h"
#include "IVideoStream.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "ExtensionAccess.h"
#include "VideoSourceReader.h"
#include "CamaroDual.h"
#include "ExtensionVCDevice.h"

namespace TopGear
{
	namespace Win
	{
		template<class T>
		class CameraFactory : public ICameraFactory<T> 
		{
		public:
			template<class U>
			static std::shared_ptr<IVideoStream> CreateInstance(U &device);
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

		template <>
		template <>
		inline std::shared_ptr<IVideoStream> CameraFactory<Camaro>::
			CreateInstance<IGenericVCDevicePtr>(IGenericVCDevicePtr& device)
		{
			auto exDevice = std::dynamic_pointer_cast<IDiscernible<IExtensionLite>>(device);
			if (exDevice == nullptr)
				return{};

			auto reader = VideoSourceReader::CreateVideoStream(device);
			if (reader == nullptr)
				return{};

			auto validator = std::dynamic_pointer_cast<ExtensionFilterBase>(exDevice->GetValidator());
			if (validator == nullptr)
				return{};
			auto ex = std::static_pointer_cast<IExtensionAccess>(
				std::make_shared<ExtensionAccess>(validator));

			auto it = CameraProfile::Repository.find(Camera::Camaro);
			if (it != CameraProfile::Repository.end())
				return std::make_shared<Camaro>(reader, ex, it->second);
			return std::make_shared<Camaro>(reader, ex);
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

		template<class T>
		template<class U>
		inline std::shared_ptr<IVideoStream> CameraFactory<T>::CreateInstance(U& device)
		{
			throw std::exception("Unimplementation");
			//return{};
		}
	}
}
