#pragma once
#include <memory>
#include <vector>
#include "IVideoStream.h"

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
#define DEEPCAM_API DEEPCAM_API_EXPORT
#else
#define DEEPCAM_API DEEPCAM_API_IMPORT
#endif
#else
#define DEEPCAM_API
#ifdef DEEPCAM_API_LOCAL
#undef DEEPCAM_API_LOCAL
#endif
#define DEEPCAM_API_LOCAL
#endif
#endif

namespace TopGear
{
	enum class Camera
	{
		StandardUVC,
		Camaro,
		CamaroDual,
		ImpalaE,
	};

	enum class Level
	{
		Info = 2,
		Warning = 4,
		Error = 5,
		Off = 9
	};

	struct LogType
	{
		static const uint8_t Standard = 0x01;
		static const uint8_t DailyFile = 0x02;
#ifdef __linux__
		static const uint8_t Syslog = 0xf0;
#endif
	private:
		LogType() = default;
	};

	// ReSharper disable CppFunctionIsNotImplemented

	class DEEPCAM_API DeepCamAPI
	{
	public:
		static DeepCamAPI &Instance();

		std::shared_ptr<IVideoStream> CreateCamera(Camera camera);

		void SetLogLevel(Level level) const;
		void EnableLog(uint8_t flag) const;
		void WriteLog(Level level, const std::string &text) const;

		template<class T>
		static std::shared_ptr<T> QueryInterface(std::shared_ptr<IVideoStream> &vs);

		~DeepCamAPI();
	private:
		DeepCamAPI();
		DeepCamAPI(DeepCamAPI &) = delete;
	};

	// ReSharper restore CppFunctionIsNotImplemented
}
