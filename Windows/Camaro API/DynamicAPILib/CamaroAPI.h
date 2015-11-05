#ifndef CAMARO_API_H
#define CAMARO_API_H

#ifdef _WIN32
#include <winsock.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif



// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
#define CAMARO_API_IMPORT __declspec(dllimport)
#define CAMARO_API_EXPORT __declspec(dllexport)
#define CAMARO_API_LOCAL
#else
#if __GNUC__ >= 4
#define CAMARO_API_IMPORT __attribute__ ((visibility ("default")))
#define CAMARO_API_EXPORT __attribute__ ((visibility ("default")))
#define CAMARO_API_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define CAMARO_API_IMPORT
#define CAMARO_API_EXPORT
#define CAMARO_API_LOCAL
#endif
#endif

#ifdef _USRDLL // defined if compiled as a DLL
#ifdef CAMAROAPILIB_EXPORTS
#define CAMARO_API CAMARO_API_EXPORT
#else
#define CAMARO_API CAMARO_API_IMPORT
#endif
#else
#define CAMARO_API
#define CAMARO_API_LOCAL
#endif


#ifdef __cplusplus
extern "C" {
#endif

	typedef void* HDevice;
	typedef void* HCamera;
	typedef void* HFrameBuffer;

	struct VideoFormat
	{
		int Width;
		int Height;
		int MaxRate;
		char PixelFormat[4];	//FourCC
	};

	struct VideoFrame
	{
		HFrameBuffer buffer;
		unsigned int width;
		unsigned int height;
		unsigned int stride;
		unsigned short index;
		timeval timestamp;
	};

	typedef void(*FnFrameCB)(VideoFrame *, int);

	enum VIDEO_DEVICE_TYPE
	{
		VIDEO_DEVICE_TYPE_GENERIC,
		VIDEO_DEVICE_TYPE_STANDARD,
		VIDEO_DEVICE_TYPE_DISCERNIBLE,
	};

	enum CAMERA_TYPE
	{
		CAMERA_TYPE_STANDARD,
		CAMERA_TYPE_CAMARO,
		CAMERA_TYPE_CAMARO_DUAL,
	};

	CAMARO_API int EnumerateDevices(VIDEO_DEVICE_TYPE type, HDevice **pList, int *pNum);
	CAMARO_API void ReleaseDevice(HDevice *phDevice);
	CAMARO_API void ReleaseDeviceList(HDevice *list, int num);

	CAMARO_API int CreateCamera(CAMERA_TYPE type, HCamera *phCamera, HDevice *pSource, int num);
	CAMARO_API void ReleaseCamera(HCamera *phCamera);

	CAMARO_API int StartStream(HCamera camera, int formatIndex);
	CAMARO_API int StopStream(HCamera camera);
	CAMARO_API int IsStreaming(HCamera camera);
	CAMARO_API int GetOptimizedFormatIndex(HCamera camera, VideoFormat *pFormat, const char *fourcc);
	CAMARO_API int GetMatchedFormatIndex(HCamera camera, const VideoFormat *pFormat);

	CAMARO_API void RegisterFrameCallback(FnFrameCB cb);
	CAMARO_API void ReleaseVideoFrames(VideoFrame *payload, int num);
	CAMARO_API void LockBuffer(VideoFrame *payload, unsigned char **ppData);
	CAMARO_API void UnlockBuffer(VideoFrame *payload);

#ifdef __cplusplus
}
#endif

#endif //CAMARO_API_H