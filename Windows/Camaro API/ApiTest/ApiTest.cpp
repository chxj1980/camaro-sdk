#include "stdafx.h"

// ReSharper disable CppUnusedIncludeDirective

#include <iostream>
#include <thread>
#include <conio.h>

#include "GenericVCDevice.h"
#include "System.h"
#include "DeviceFactory.h"
#include "CameraFactory.h"
#include "Camaro.h"
#include "CamaroDual.h"
#include "StandardUVC.h"
// ReSharper restore CppUnusedIncludeDirective

void OnFrameCB(std::vector<TopGear::IVideoFrameRef> &frames)
{
	if (frames.size() == 0)
		return;
	auto frame = frames[0];
	uint8_t *pData;		//Frame data
	uint32_t stride;	//Actual stride of frame	
	frame->LockBuffer(&pData, &stride);		//Lock memory
	std::cout << "FrameIdx: " << int(frame->GetFrameIdx()) << std::endl;
	//std::cout << static_cast<int>(frame->GetTimestamp().tv_usec) << std::endl;
	//Do something...
	frame->UnlockBuffer();	//Unlock memory
}

class FrameDemo //: public TopGear::IVideoFrameCallback
{
public:
	//virtual void OnFrame(std::vector<std::shared_ptr<TopGear::IVideoFrame>> &frames) override
	static void OnFrameS(std::vector<TopGear::IVideoFrameRef> &frames)
	{
		OnFrameCB(frames);
	}
	void OnFrameMember(std::vector<TopGear::IVideoFrameRef> &frames) const
	{
		OnFrameCB(frames);
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
//#define CAMARO_SOLO
//#define 	STD_UVC

void main()
{
	FrameDemo demo;
	TopGear::Win::System::Initialize();
#ifdef STD_UVC
	auto uvcDevices = TopGear::Win::DeviceFactory<TopGear::Win::StandardVCDevice>::EnumerateDevices();
	std::shared_ptr<TopGear::IVideoStream> uvc;
	for (auto dev : uvcDevices)
	{
		std::cout << dev->GetFriendlyName() << std::endl;
		std::cout << dev->GetSymbolicLink() << std::endl;
		uvc = TopGear::Win::CameraFactory<TopGear::StandardUVC>::CreateInstance(dev);
		if (uvc)
		{
			TopGear::IVideoStream::RegisterFrameCallback(*uvc, &FrameDemo::OnFrame);
			TopGear::VideoFormat format;
			auto index = uvc->GetOptimizedFormatIndex(format);
			uvc->StartStream(index);
			break;
		}
	}
	if (uvc)
	{
		Loop();
		uvc->StopStream();
	}
#elif defined CAMARO_SOLO
	auto devices = TopGear::Win::DeviceFactory<TopGear::Win::DiscernibleVCDevice>::EnumerateDevices();
	std::shared_ptr<TopGear::IVideoStream> camaro;
	for (auto item : devices)
	{
		//Show some basic info about device
		std::cout << item->GetFriendlyName() << std::endl;
		std::cout << item->GetSymbolicLink() << std::endl;
		std::cout << item->GetDeviceInfo() << std::endl;
		//Create a Camaro instance 
		camaro = TopGear::Win::CameraFactory<TopGear::Camaro>::CreateInstance(item);
		auto ioControl = std::dynamic_pointer_cast<TopGear::IDeviceControl>(camaro);
		auto cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camaro);
		if (camaro && ioControl && cameraControl)
		{
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
				camaro->StartStream(index);
				break;
			}
			else
				camaro.reset();
		}
	}
	if (camaro)
	{
		Loop();
		camaro->StopStream();
	}
#elif defined CAMARO_DUAL
	auto devices = TopGear::Win::DeviceFactory<TopGear::Win::DiscernibleVCDevice>::EnumerateDevices();
	auto dual = TopGear::Win::CameraFactory<TopGear::CamaroDual>::CreateInstance(devices);
	if (dual)
	{
		TopGear::IVideoStream::RegisterFrameCallback(*dual, &FrameDemo::OnFrameMember, &demo);
		TopGear::VideoFormat format;
		auto index = dual->GetOptimizedFormatIndex(format);
		dual->StartStream(index);
		Loop();
		dual->StopStream();
	}
#endif
	
	std::cout << "Exited! " << std::endl;
	_getch();
	//std::cin.get();
	TopGear::Win::System::Dispose();
}



