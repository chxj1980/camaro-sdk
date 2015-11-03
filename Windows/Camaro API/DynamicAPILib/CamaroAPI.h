#ifndef CAMARO_API_H
#define CAMARO_API_H

#ifdef _WIN32
#include <winsock.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif

#ifdef DYNAMICAPILIB_EXPORTS
#define DYNAMICAPILIB_API __declspec(dllexport)
#else
#define DYNAMICAPILIB_API __declspec(dllimport)
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

	DYNAMICAPILIB_API int EnumerateDevices(VIDEO_DEVICE_TYPE type, HDevice **pList, int *pNum);
	DYNAMICAPILIB_API void ReleaseDevice(HDevice *phDevice);
	DYNAMICAPILIB_API void ReleaseDeviceList(HDevice *list, int num);

	DYNAMICAPILIB_API int CreateCamera(CAMERA_TYPE type, HCamera *phCamera, HDevice *pSource, int num);
	DYNAMICAPILIB_API void ReleaseCamera(HCamera *phCamera);

	DYNAMICAPILIB_API int StartStream(HCamera camera, int formatIndex);
	DYNAMICAPILIB_API int StopStream(HCamera camera);
	DYNAMICAPILIB_API int IsStreaming(HCamera camera);
	DYNAMICAPILIB_API int GetOptimizedFormatIndex(HCamera camera, VideoFormat *pFormat, const char *fourcc);
	DYNAMICAPILIB_API int GetMatchedFormatIndex(HCamera camera, const VideoFormat *pFormat);

	DYNAMICAPILIB_API void RegisterFrameCallback(FnFrameCB cb);
	DYNAMICAPILIB_API void ReleaseVideoFrames(VideoFrame *payload, int num);
	DYNAMICAPILIB_API void LockBuffer(VideoFrame *payload, unsigned char **ppData);
	DYNAMICAPILIB_API void UnlockBuffer(VideoFrame *payload);

#ifdef __cplusplus
}
#endif

#endif //CAMARO_API_H