// ApiDynamicTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _USRDLL

// ReSharper disable CppUnusedIncludeDirective

#include <iostream>
#include <thread>
#include <conio.h>

#include "DeepCamAPI.h"
#include "IVideoStream.h"
#include "IVideoFrame.h"
#include "ICameraControl.h"
#include "ILowlevelControl.h"
#include "IDeviceControl.h"
#include "IMultiVideoStream.h"
#include "VideoFormat.h"

// ReSharper restore CppUnusedIncludeDirective

void OnFrameCB(TopGear::IVideoStream &stream, std::vector<TopGear::IVideoFramePtr> &frames)
{
	if (frames.size() == 0)
		return;
	auto frame = frames[0];
	uint8_t *pData;		//Frame data
	uint32_t stride;	//Actual stride of frame	
	uint8_t *pExtra;

	frame->LockBuffer(&pData, &stride, &pExtra);		//Lock memory
	std::cout << "FrameIdx: " << int(frame->GetFrameIdx()) << std::endl;
	//std::cout << static_cast<int>(frame->GetTimestamp().tv_usec) << std::endl;
	//Do something...
	frame->UnlockBuffer();	//Unlock memory
}

class FrameDemo //: public TopGear::IVideoFrameCallback
{
public:
	//virtual void OnFrame(std::vector<std::shared_ptr<TopGear::IVideoFrame>> &frames) override
	static void OnFrameS(TopGear::IVideoStream &stream, std::vector<TopGear::IVideoFramePtr> &frames)
	{
		OnFrameCB(stream, frames);
	}
	void OnFrameMember(TopGear::IVideoStream &stream, std::vector<TopGear::IVideoFramePtr> &frames) const
	{
		OnFrameCB(stream, frames);
	}
};



void Loop()
{
	char c;
	std::cout << "Press esc to exit loop! " << std::endl;
	while (true)
	{
		c = _getch();
		if (c == 27)
			break;
	}
}

#define CAMARO_DUAL 
#define CAMARO_SOLO
#define 	STD_UVC

void main()
{
	FrameDemo demo;
	auto deepcam = TopGear::DeepCamAPI::Instance();
#ifdef STD_UVC
	//auto uvcDevices = deepcam.EnumerateDevices(TopGear::DeviceType::Standard);
	std::shared_ptr<TopGear::IVideoStream> uvc;
	//for (auto dev : uvcDevices)
	//{
	//	std::cout << dev->GetFriendlyName() << std::endl;
	//	std::cout << dev->GetSymbolicLink() << std::endl;
		uvc = deepcam.CreateCamera(TopGear::Camera::StandardUVC);
		if (uvc)
		{
			auto formats = uvc->GetAllFormats();
			for (auto i = 0; i < formats.size(); ++i)
			{
				std::cout << i << ": " << formats[i].Width << "X" << formats[i].Height << " @ " << formats[i].MaxRate << "fps "
					<< std::string(formats[i].PixelFormat, 4) << std::endl;
			}
			TopGear::IVideoStream::RegisterFrameCallback(*uvc, &FrameDemo::OnFrameMember, &demo);
			TopGear::VideoFormat format;
			auto index = uvc->GetOptimizedFormatIndex(format);
			uvc->SetCurrentFormat(index);
			uvc->StartStream();
			//break;
		}
	//}
	if (uvc)
	{
		Loop();
		uvc->StopStream();
	}
#elif defined CAMARO_SOLO
	//auto devices = deepcam.EnumerateDevices(TopGear::DeviceType::DeepGlint);
	std::shared_ptr<TopGear::IVideoStream> camaro;
	//for (auto item : devices)
	//{
	//	//Show some basic info about device
	//	std::cout << item->GetFriendlyName() << std::endl;
	//	std::cout << item->GetSymbolicLink() << std::endl;
	//	std::cout << item->GetDeviceInfo() << std::endl;
		//Create a Camaro instance 
		camaro = deepcam.CreateCamera(TopGear::Camera::Camaro);
		auto ioControl = deepcam.QueryInterface<TopGear::IDeviceControl>(camaro);
		auto cameraControl = deepcam.QueryInterface<TopGear::ICameraControl>(camaro);
		if (camaro && ioControl && cameraControl)
		{
			auto formats = camaro->GetAllFormats();
			for (auto i = 0; i < formats.size(); ++i)
			{
				std::cout << i << ": " << formats[i].Width << "X" << formats[i].Height << " @ " << formats[i].MaxRate << "fps "
					<< std::string(formats[i].PixelFormat, 4) << std::endl;
			}
			//Register callback function for frame arrival
			TopGear::IVideoStream::RegisterFrameCallback(*camaro, &FrameDemo::OnFrameMember, &demo);
			//Is master camaro
			if (ioControl->QueryDeviceRole() == 0)
			{
				TopGear::VideoFormat format;
				//Get optimized video format
				auto index = camaro->GetOptimizedFormatIndex(format);
				std::cout << "Auto selected format: " << format.Width << "X" << format.Height << " @ " << format.MaxRate << "fps ";
				std::cout << std::string(format.PixelFormat, 4) << std::endl;
				//Start streaming with selected format index
				camaro->SetCurrentFormat(index);
				camaro->StartStream();
			//break;
			}
			else
				camaro.reset();
		}
	//}
	if (camaro)
	{
		Loop();
		camaro->StopStream();
	}
#elif defined CAMARO_DUAL
	//auto devices = deepcam.EnumerateDevices(TopGear::DeviceType::DeepGlint);
	auto dual = deepcam.CreateCamera(TopGear::Camera::CamaroDual);
	if (dual)
	{
		TopGear::IVideoStream::RegisterFrameCallback(*dual, &FrameDemo::OnFrameMember, &demo);
		TopGear::VideoFormat format;
		auto index = dual->GetOptimizedFormatIndex(format);
		dual->SetCurrentFormat(index);
		dual->StartStream();
		Loop();
		dual->StopStream();
	}
#endif

	std::cout << "Exited! " << std::endl;
	//_getch();
	//std::cin.get();
	//TopGear::DeepCamAPI::Instance().Dispose();
}





