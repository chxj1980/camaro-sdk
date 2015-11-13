#pragma once
#include <memory>
#include <vector>
#include "IVideoStream.h"
#include "IGenericVCDevice.h"

// Generic helper definitions for shared library support
#ifndef DEEPCAM_API
#if defined _WIN32 || defined __CYGWIN__
#define DEEPCAM_API_IMPORT __declspec(dllimport)
#define DEEPCAM_API_EXPORT __declspec(dllexport)
#define DEEPCAM_API_LOCAL
#else
#if __GNUC__ >= 4
#define DEEPCAM_API_IMPORT __attribute__ ((visibility ("default")))
#define DEEPCAM_API_EXPORT __attribute__ ((visibility ("default")))
#define DEEPCAM_API_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define DEEPCAM_API_IMPORT
#define DEEPCAM_API_EXPORT
#define DEEPCAM_API_LOCAL
#endif
#endif

#ifdef _USRDLL // defined if compiled as a DLL
#ifdef DEEPCAMAPILIB_EXPORTS
#define DEEPCAM_API CAMARO_API_EXPORT
#else
#define DEEPCAM_API CAMARO_API_IMPORT
#endif
#else
#define DEEPCAM_API
#define DEEPCAM_API_LOCAL
#endif
#endif

namespace TopGear
{
	enum class DeviceType
	{
		Generic,
		Standard,
		DeepGlint,
		Etron,
	};

	enum class Camera
	{
		StandardUVC,
		Camaro,
		CamaroDual,
		Etron3D,
	};


	class DEEPCAM_API DeepCamAPI
	{
	public:
		static DeepCamAPI &Instance();
		std::vector<IGenericVCDeviceRef> EnumerateDevices(DeviceType type) const;

		template<class U>
		std::shared_ptr<IVideoStream> CreateCamera(Camera camera, U &source);

		template<class T>
		std::shared_ptr<T> QueryInterface(std::shared_ptr<IVideoStream> &vs) const;
		
		~DeepCamAPI();
	private:
		DeepCamAPI();
	};
}
