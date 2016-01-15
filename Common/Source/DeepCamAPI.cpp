#include "DeepCamAPI.h"
#include "DeviceFactory.h"
#include "GenericVCDevice.h"
#include "DGExtensionFilter.h"
#include "EtronExtensionFilter.h"
#include "CameraFactory.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "CamaroDual.h"
#include "IMultiVideoStream.h"
#include "StandardVCDevice.h"
#include "CameraProfile.h"
#include "Logger.h"

#ifdef _WIN32
	#include "System.h"
	#include <Windows.h>
	#include <strsafe.h>
	using namespace TopGear::Win;
#elif defined(__linux__)
	#include <unistd.h>
	#include <dirent.h>
	using namespace TopGear::Linux;
#endif

#include <fstream>
#include <iostream>

namespace TopGear
{
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

#ifdef _WIN32
		static const std::wstring ConfigSuffix;
#elif defined(__linux__)
		static const std::string ConfigSuffix;
#endif

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
			case Camera::ImpalaE:
				vs = CameraFactory<ImpalaE>::CreateInstance(source);
				break;
			default:
				break;
			}
			return vs;
		}

		static void LoadCameraConfigs()
		{
#ifdef _WIN32
			auto dir = new wchar_t[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, dir);
			StringCchCat(dir, MAX_PATH, TEXT("\\*"));

			WIN32_FIND_DATA ffd;
			auto hFind = FindFirstFile(dir, &ffd);
			if (hFind == INVALID_HANDLE_VALUE)
				return;
			do
			{
				if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
					(ffd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
				{
					std::wstring filename(ffd.cFileName);
					auto offset = int(filename.length()) - int(ConfigSuffix.length());
					if (offset < 0 || filename.substr(offset) != ConfigSuffix)
						continue;
					std::ifstream config_doc(filename, std::ifstream::binary);
					if (config_doc.is_open())
					{
						CameraProfile::Parse(config_doc);
						config_doc.close();
					}
				}
			} while (FindNextFile(hFind, &ffd));
#elif defined(__linux__)
            //Get current path
            char cCurrentPath[FILENAME_MAX];
            if (!getcwd(cCurrentPath, sizeof(cCurrentPath)))
                return; //errno
            cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */

            dirent *dirp;
            DIR *dp = opendir(cCurrentPath);
            if (dp == nullptr)
            {
                //cout << "Error(" << errno << ") opening " << dir << endl;
                return;
            }

            while ((dirp = readdir(dp)) != nullptr)
            {
                if (dirp->d_type == DT_DIR)
                    continue;
                auto filename = std::string(dirp->d_name);
                int offset = filename.length() - ConfigSuffix.length();
                if (offset < 0 || filename.substr(offset) != ConfigSuffix)
                    continue;
                std::ifstream config_doc(filename, std::ifstream::binary);
                if (config_doc.is_open())
                {
                    CameraProfile::Parse(config_doc);
                    config_doc.close();
                }
            }
            closedir(dp);
#endif
        }
	};

#ifdef _WIN32
	const std::wstring DeepCamAPIInternal::ConfigSuffix = L".profile.json";
#elif defined(__linux__)
	const std::string DeepCamAPIInternal::ConfigSuffix = ".profile.json";
#endif

	std::unique_ptr<DeepCamAPI> DeepCamAPIInternal::Instance;

	DEEPCAM_API DeepCamAPI &DeepCamAPI::Instance()
	{
		if (DeepCamAPIInternal::Instance == nullptr)
			DeepCamAPIInternal::Instance = std::unique_ptr<DeepCamAPI>(new DeepCamAPI);
		return *DeepCamAPIInternal::Instance;
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	DEEPCAM_API void DeepCamAPI::SetLogLevel(Level level) const
	{
		spdlog::set_level(spdlog::level::level_enum(int(level)));
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	DEEPCAM_API void DeepCamAPI::EnableLog(uint8_t flag) const
	{
		Logger::SwitchStdout((flag & LogType::Standard) != 0);
		Logger::SwitchDaily((flag & LogType::DailyFile) != 0);
#ifdef __linux__
		Logger::SwitchSyslog((flag & LogType::Syslog) != 0);
#endif
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	DEEPCAM_API void DeepCamAPI::WriteLog(Level level, const std::string &text) const
	{
		Logger::Write(spdlog::level::level_enum(int(level)), text);
	}

	DeepCamAPI::DeepCamAPI()
	{
#ifdef _WIN32
		System::Initialize();
#endif
		Logger::Initialize();
		DeepCamAPIInternal::LoadCameraConfigs();
	}

	DeepCamAPI::~DeepCamAPI()
	{
#ifdef _WIN32
		System::Dispose();
#endif
	}


	template<>
	DEEPCAM_API std::shared_ptr<TopGear::ICameraControl> DeepCamAPI::QueryInterface<TopGear::ICameraControl>(std::shared_ptr<IVideoStream> &vs)
	{
		return std::dynamic_pointer_cast<TopGear::ICameraControl>(vs);
	}

	template<>
	DEEPCAM_API std::shared_ptr<IDeviceControl> DeepCamAPI::QueryInterface<IDeviceControl>(std::shared_ptr<IVideoStream> &vs)
	{
		return std::dynamic_pointer_cast<IDeviceControl>(vs);
	}

	template<>
	DEEPCAM_API std::shared_ptr<ILowlevelControl> DeepCamAPI::QueryInterface<ILowlevelControl>(std::shared_ptr<IVideoStream> &vs)
	{
		return std::dynamic_pointer_cast<ILowlevelControl>(vs);
	}

	template<>
	DEEPCAM_API std::shared_ptr<IMultiVideoStream> DeepCamAPI::QueryInterface<IMultiVideoStream>(std::shared_ptr<IVideoStream> &vs)
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
			if (inventory.size() > 0)
			{
				vs = CameraFactory<StandardUVC>::CreateInstance(inventory[0]);
				if (vs)
					Logger::Write(spdlog::level::info, "Standard UVC camera created");
			}
			break;
		case Camera::Camaro:
			inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::DeepGlint);
			for (auto device : inventory)
			{
				vs = CameraFactory<Camaro>::CreateInstance(device);
				auto dc = std::dynamic_pointer_cast<IDeviceControl>(vs);
				if (dc)
				{
					PropertyData<uint8_t> role;
					dc->GetControl("DeviceRole", role);
					if (role.Payload == 0)
					{
						Logger::Write(spdlog::level::info, "Camaro camera created");
						break;
					}
					else
						Logger::Write(spdlog::level::warn, "Camaro camera found, but in slave mode");
				}
				vs.reset();
			}
			break;
		case Camera::CamaroDual:
			inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::DeepGlint);
			vs = CameraFactory<CamaroDual>::CreateInstance(inventory);
			if (vs)
				Logger::Write(spdlog::level::info, "Camaro stereo camera created");
			break;
		case Camera::ImpalaE:
			inventory = DeepCamAPIInternal::EnumerateDevices(DeviceType::Etron);
			for (auto device : inventory)
			{
				vs = CameraFactory<ImpalaE>::CreateInstance(device);
				if (vs)
				{
					Logger::Write(spdlog::level::info, "ImpalaE stereo camera created");
					break;
				}
			}
			break;
		default:
			break;
		}
		return vs;
	}
}

