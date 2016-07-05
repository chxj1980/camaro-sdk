#include "DeepCamAPI.h"
#include "DeviceFactory.h"
#include "GenericVCDevice.h"
#include "DGExtensionFilter.h"
#include "EtronExtensionFilter.h"
#include "CameraFactory.h"
#include "StandardUVC.h"
#include "Camaro.h"
#include "CamaroDual.h"
#include "ImpalaE.h"
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

	class DeepCamAPIInternal
	{
    public:
#ifdef _WIN32
		static const std::wstring ConfigSuffix;
#elif defined(__linux__)
		static const std::string ConfigSuffix;
#endif

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

	DEEPCAM_API DeepCamAPI &DeepCamAPI::Instance()
	{
        static std::unique_ptr<DeepCamAPI> instance;
        if (instance == nullptr)
            instance = std::unique_ptr<DeepCamAPI>(new DeepCamAPI);
        return *instance;
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

    DEEPCAM_API std::vector<IGenericVCDevicePtr> DeepCamAPI::EnumerateDevices(DeviceCategory type)
    {
        std::vector<IGenericVCDevicePtr> inventory;
        switch (type)
        {
        case DeviceCategory::Generic:
            inventory = DeviceFactory<GenericVCDevice>::EnumerateDevices();
            break;
        case DeviceCategory::Standard:
            inventory = DeviceFactory<StandardVCDevice>::EnumerateDevices();
            break;
        case DeviceCategory::DeepGlint:
            inventory = DeviceFactory<ExtensionVCDevice<DGExtensionFilter>>::EnumerateDevices();
            break;
        case DeviceCategory::Etron:
            inventory = DeviceFactory<ExtensionVCDevice<EtronExtensionFilter>>::EnumerateDevices();
            break;
#ifdef SUPPORT_POINTGREY
        case DeviceCategory::FlyCapture:
            inventory = DeviceFactory<FlyCaptureDevice>::EnumerateDevices();
            break;
#endif
        default:
            break;
        }
        return inventory;
    }


    template<>
    DEEPCAM_API std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<IGenericVCDevicePtr>
        (Camera camera, IGenericVCDevicePtr & source)
    {
        std::shared_ptr<IVideoStream> vs;
        std::shared_ptr<IDeviceControl> dc;
        switch (camera)
        {
        case Camera::StandardUVC:
            vs = CameraFactory<StandardUVC>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "Standard UVC camera created");
            break;
        case Camera::Camaro:
            vs = CameraFactory<Camaro>::CreateInstance(source);
            dc = std::dynamic_pointer_cast<IDeviceControl>(vs);
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
            break;
        case Camera::CamaroISP:
            vs = CameraFactory<CamaroISP>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "CamaroISP camera created");
            break;
        case Camera::CamaroMovidius:
            vs = CameraFactory<CamaroMovidius>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "CamaroMovidius camera created");
            break;
        case Camera::ImpalaE:
            vs = CameraFactory<ImpalaE>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "ImpalaE stereo camera created");
            break;
#ifdef SUPPORT_POINTGREY
        case Camera::PointGrey:
            vs = CameraFactory<PointGrey>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "PointGrey camera created");
            break;
#endif
#ifdef SUPPORT_JPEG_SEQUENCE
        case Camera::JpegSequence:
            vs = CameraFactory<ImageSequence>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "Virtual camera created");
            break;
#endif
        case Camera::CamaroDual:
            break;
        case Camera::Fovea:
            break;
        default:
            break;
        }
        return vs;
    }

    template<>
    DEEPCAM_API std::shared_ptr<IVideoStream> DeepCamAPI::CreateCamera<std::vector<IGenericVCDevicePtr>>
        (Camera camera, std::vector<IGenericVCDevicePtr> & source)
    {
        std::shared_ptr<IVideoStream> vs;
        std::vector<std::shared_ptr<IVideoStream>> vss;
        switch (camera)
        {
        case Camera::CamaroDual:
            vs = CameraFactory<CamaroDual>::CreateInstance(source);
            if (vs)
                Logger::Write(spdlog::level::info, "Camaro stereo camera created");
            break;
        case Camera::Fovea:
            if (source.size()<2)
                break;

            if (std::dynamic_pointer_cast<StandardVCDevice>(source[0]))
                vss.push_back(CreateCamera(Camera::StandardUVC, source[0]));
#ifdef SUPPORT_POINTGREY
            else if (std::dynamic_pointer_cast<FlyCaptureDevice>(source[0]))
                vss.push_back(CreateCamera(Camera::PointGrey, source[0]));
#endif
#ifdef SUPPORT_JPEG_SEQUENCE
            else if (std::dynamic_pointer_cast<ImageDevice>(source[0]))
                vss.push_back(CreateCamera(Camera::JpegSequence, source[0]));
#endif
            else
                vss.push_back(CreateCamera(Camera::CamaroISP, source[0]));

            if (std::dynamic_pointer_cast<StandardVCDevice>(source[1]))
                vss.push_back(CreateCamera(Camera::StandardUVC, source[1]));
#ifdef SUPPORT_POINTGREY
            else if (std::dynamic_pointer_cast<FlyCaptureDevice>(source[1]))
                vss.push_back(CreateCamera(Camera::PointGrey, source[1]));
#endif
#ifdef SUPPORT_JPEG_SEQUENCE
            else if (std::dynamic_pointer_cast<ImageDevice>(source[1]))
                vss.push_back(CreateCamera(Camera::JpegSequence, source[1]));
#endif
            else
                vss.push_back(CreateCamera(Camera::CamaroISP, source[1]));

            vs = CameraFactory<Fovea>::CreateInstance(vss);
            if (vs)
                Logger::Write(spdlog::level::info, "Fovea camera created");
            break;
        default:
            break;
        }
        return vs;
    }
}

