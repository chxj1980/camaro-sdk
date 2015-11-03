// DynamicAPILib.cpp : Defines the exported functions for the DLL application.
//

#include "CamaroAPI.h"
#include <GenericVCDevice.h>
#include <DeviceFactory.h>
#include <CameraFactory.h>
#include <Camaro.h>
#include <StandardUVC.h>
#include <CamaroDual.h>


typedef TopGear::IGenericVCDeviceRef* IGenericVCDeviceRefPtr;

DYNAMICAPILIB_API int EnumerateDevices(VIDEO_DEVICE_TYPE type, HDevice **pList, int *pNum)
{
	*pNum = 0;
	*pList = nullptr;
	std::vector<TopGear::IGenericVCDeviceRef> devices;
	switch (type)
	{
	case VIDEO_DEVICE_TYPE_GENERIC:
		devices = TopGear::Win::DeviceFactory<TopGear::Win::GenericVCDevice>::EnumerateDevices();
		break;
	case VIDEO_DEVICE_TYPE_STANDARD:
		devices = TopGear::Win::DeviceFactory<TopGear::Win::StandardVCDevice>::EnumerateDevices();
		break;
	case VIDEO_DEVICE_TYPE_DISCERNIBLE:
		devices = TopGear::Win::DeviceFactory<TopGear::Win::DiscernibleVCDevice>::EnumerateDevices();
		break;
	default:
		return -1;
	}
	*pNum = static_cast<int>(devices.size());
	if (*pNum == 0)
		return 0;
	auto pDevicelist = new IGenericVCDeviceRefPtr[*pNum];
	auto i = 0;
	for (auto dev : devices)
		pDevicelist[i++] = new TopGear::IGenericVCDeviceRef(dev);
	*pList = reinterpret_cast<HDevice *>(pDevicelist);
	return 0;
}

DYNAMICAPILIB_API void ReleaseDevice(HDevice *phDevice)
{
	if (*phDevice !=nullptr)
	{
		auto ptr = reinterpret_cast<IGenericVCDeviceRefPtr>(*phDevice);
		ptr->reset();
		delete ptr;
		*phDevice = nullptr;
	}
}

DYNAMICAPILIB_API void ReleaseDeviceList(HDevice *list, int num)
{
	if (list != nullptr)
	{
		for (auto i = 0; i < num; ++i)
			ReleaseDevice(list+i);
		delete[] list;
	}
}

DYNAMICAPILIB_API int CreateCamera(CAMERA_TYPE type, HCamera *phCamera, HDevice *pSource, int num)
{
	*phCamera = nullptr;
	IGenericVCDeviceRefPtr ptr;
	std::shared_ptr<TopGear::IVideoStream> *vstream;
	std::vector<TopGear::IGenericVCDeviceRef> list;
	switch(type)
	{
	case CAMERA_TYPE_STANDARD: 
		ptr = reinterpret_cast<IGenericVCDeviceRefPtr>(*pSource);
		vstream = new std::shared_ptr<TopGear::IVideoStream>(
			TopGear::Win::CameraFactory<TopGear::StandardUVC>::CreateInstance(*ptr));
		break;
	case CAMERA_TYPE_CAMARO:
		ptr = reinterpret_cast<IGenericVCDeviceRefPtr>(*pSource);
		vstream = new std::shared_ptr<TopGear::IVideoStream>(
			TopGear::Win::CameraFactory<TopGear::Camaro>::CreateInstance(*ptr));
		break;
	case CAMERA_TYPE_CAMARO_DUAL:
		if (num <= 0)
			return 0;
		for (auto i = 0; i < num; ++i)
		{
			ptr= reinterpret_cast<IGenericVCDeviceRefPtr>(pSource[i]);
			list.push_back(*ptr);
		}
		vstream = new std::shared_ptr<TopGear::IVideoStream>(
			TopGear::Win::CameraFactory<TopGear::CamaroDual>::CreateInstance(list));
		break;
	default: 
		return -1;
	}
	if (*vstream)
	{
		*phCamera = vstream;
	}
	return 0;
}

DYNAMICAPILIB_API void ReleaseCamera(HCamera *phCamera)
{
	if(*phCamera != nullptr)
	{
		auto ptr = reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(*phCamera);
		ptr->reset();
		delete ptr;
		*phCamera = nullptr;
	}
}

void OnFrameCB(std::vector<TopGear::IVideoFrameRef> &frames);

DYNAMICAPILIB_API int StartStream(HCamera camera, int formatIndex)
{
	if (camera == nullptr)
		return 0;
	auto &vs = *reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(camera);
	TopGear::IVideoStream::RegisterFrameCallback(*vs, &OnFrameCB);
	return vs->StartStream(formatIndex);
}

DYNAMICAPILIB_API int StopStream(HCamera camera)
{
	if (camera == nullptr)
		return 0;
	auto &vs = *reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(camera);
	return vs->StopStream();
}
DYNAMICAPILIB_API int IsStreaming(HCamera camera)
{
	if (camera == nullptr)
		return 0;
	auto &vs = *reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(camera);
	return vs->IsStreaming();
}

DYNAMICAPILIB_API int GetOptimizedFormatIndex(HCamera camera, VideoFormat *pFormat, const char *fourcc)
{
	if (camera == nullptr)
		return 0;
	auto &vs = *reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(camera);
	TopGear::VideoFormat f;
	auto result = vs->GetOptimizedFormatIndex(f, fourcc);
	if (pFormat)
		memcpy(pFormat, &f, sizeof(f));
	return result;
}
DYNAMICAPILIB_API int GetMatchedFormatIndex(HCamera camera, const VideoFormat *pFormat)
{
	if (camera == nullptr)
		return 0;
	auto &vs = *reinterpret_cast<std::shared_ptr<TopGear::IVideoStream> *>(camera);
	TopGear::VideoFormat f;
	memcpy(&f, pFormat, sizeof(f));
	return vs->GetMatchedFormatIndex(f);
}

FnFrameCB onFrame = nullptr;

DYNAMICAPILIB_API void RegisterFrameCallback(FnFrameCB cb)
{
	onFrame = cb;
}

void OnFrameCB(std::vector<TopGear::IVideoFrameRef> &frames)
{
	if (onFrame == nullptr)
		return;
	auto vfs = new VideoFrame[frames.size()];
	auto i = 0;
	for (auto f : frames)
	{
		vfs[i].buffer = new TopGear::IVideoFrameRef(f);
		f->QueryActualSize(vfs[i].width, vfs[i].height);
		vfs[i].index = f->GetFrameIdx();
		vfs[i].timestamp = f->GetTimestamp();
		vfs[i++].stride = 0;
	}
	onFrame(vfs, static_cast<int>(frames.size()));
}

DYNAMICAPILIB_API void ReleaseVideoFrames(VideoFrame *payload, int num)
{
	for (auto i = 0; i < num; ++i)
	{
		auto pFrame = reinterpret_cast<TopGear::IVideoFrameRef *>(payload[i].buffer);
		pFrame->reset();
		delete pFrame;
	}
	delete[] payload;
}

DYNAMICAPILIB_API void LockBuffer(VideoFrame *payload, unsigned char **ppData)
{
	auto pFrame = reinterpret_cast<TopGear::IVideoFrameRef *>(payload->buffer);
	(*pFrame)->LockBuffer(ppData, &payload->stride);
}

DYNAMICAPILIB_API void UnlockBuffer(VideoFrame *payload)
{
	auto pFrame = reinterpret_cast<TopGear::IVideoFrameRef *>(payload->buffer);
	(*pFrame)->UnlockBuffer();
}
