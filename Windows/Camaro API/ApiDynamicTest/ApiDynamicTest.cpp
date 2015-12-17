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

inline int fast_abs(int value)
{
	uint32_t temp = value >> 31;     // make a mask of the sign bit
	value ^= temp;                   // toggle the bits if value is negative
	value += temp & 1;               // add one if value was negative
	return value;
}

const float WEIGHT_B = 0.1140f;
const float WEIGHT_G = 0.5870f;
const float WEIGHT_R = 0.2989f;
const uint16_t BAYER_MAX = 0xFF;

//AR 0134
#define Bayer(x,y)  ((unsigned short)(raw_16[(x) + w *(y)] >> 4) & 0x00FF)
//#define Bayer(x,y)  raw_16[(x) + w *(y)]
#define Grey(x,y)	grey[(x) + w *(y)]

void bayer_copy_grey(uint16_t *raw_16, uint8_t *grey, int x, int y, int w)
{
	float val = 0;

	val = Bayer(x, y + 1) * WEIGHT_B +
		Bayer(x, y) * WEIGHT_G +
		Bayer(x + 1, y) * WEIGHT_R;
	Grey(x, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

	val = Bayer(x, y + 1) * WEIGHT_B +
		((Bayer(x, y) + Bayer(x + 1, y + 1)) >> 1) * WEIGHT_G +
		Bayer(x + 1, y) * WEIGHT_R;
	Grey(x + 1, y) = Grey(x, y + 1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

	val = Bayer(x, y + 1) * WEIGHT_B +
		Bayer(x + 1, y + 1) * WEIGHT_G +
		Bayer(x + 1, y) * WEIGHT_R;
	Grey(x + 1, y + 1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);
}


void bayer_bilinear_grey(uint16_t *raw_16, uint8_t *grey, int x, int y, int w)
{
	float val = 0;

	val = ((Bayer(x, y - 1) + Bayer(x, y + 1)) >> 1) * WEIGHT_B +
		Bayer(x, y) * WEIGHT_G +
		((Bayer(x - 1, y) + Bayer(x + 1, y)) >> 1) * WEIGHT_R;
	Grey(x, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

	val = ((Bayer(x, y - 1) + Bayer(x + 2, y + 1) + Bayer(x + 2, y - 1) + Bayer(x, y + 1)) >> 2) * WEIGHT_B +
		((Bayer(x, y) + Bayer(x + 2, y) + Bayer(x, y) + Bayer(x + 2, y)) >> 2) * WEIGHT_G +
		Bayer(x + 1, y)* WEIGHT_R;
	Grey(x+1, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

	val = Bayer(x, y + 1) * WEIGHT_B +
		((Bayer(x, y) + Bayer(x + 1, y + 1) + Bayer(x - 1, y + 1) + Bayer(x, y + 2)) >> 2)* WEIGHT_G +
		((Bayer(x - 1, y) + Bayer(x + 1, y) + Bayer(x - 1, y + 2) + Bayer(x + 1, y + 2)) >> 2)* WEIGHT_R;
	Grey(x, y+1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

	val = ((Bayer(x, y + 1) + Bayer(x + 2, y + 1)) >> 1) * WEIGHT_B +
		Bayer(x + 1, y + 1)* WEIGHT_G +
		((Bayer(x + 1, y) + Bayer(x + 1, y + 2)) >> 1)* WEIGHT_R;
	Grey(x+1, y+1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);
}

void RawToGrey(uint8_t *raw, uint8_t *grey, int w, int h)
{
	for (auto j = 0; j < h; j += 2)
	{
		for (auto i = 0; i < w; i += 2)
		{

			if (i == 0 || j == 0 || i == w - 2 || j == h - 2)
				bayer_copy_grey(reinterpret_cast<uint16_t *>(raw), grey, i, j, w);
			else
				bayer_bilinear_grey(reinterpret_cast<uint16_t *>(raw), grey, i, j, w);
		}
	}
}

float Gradient(uint8_t *pixel, int x,int y, uint32_t stride, float &max)
{
	max = 0;
	float g = 0;
	auto current = pixel[y*stride + x];
	for (auto i = -1; i <= 1; ++i)
		for (auto j = -1; j <= 1;++j)
		{
			if (i == 0 && j == 0)
				continue;
			auto val = float(fast_abs(current - pixel[(y + i)*stride + (x + j)]));
			if (fast_abs(i)+ fast_abs(j)>1)
				val *= 0.707107f;
			if (val > max)
				max = val;
			g += val;
		}
	return g;
}

std::unique_ptr<uint8_t[]> pixel;

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
	int w, h;
	frame->GetSize(w, h);
	if (pixel == nullptr)
		pixel = std::unique_ptr<uint8_t[]>(new uint8_t[w*h]);
	RawToGrey(pData, pixel.get(), w, h);

	constexpr auto lamda1 = 1.0f;
	constexpr auto lamda2 = 1 - lamda1;
	float sum1 = 0;
	float sum2 = 0;
	
	for (auto i = 1; i < h - 1; ++i)
		for (auto j = 1; j < w - 1;++j)
		{
			float max = 0;
			auto g = Gradient(pixel.get(), j, i, w, max);
			sum1 += g;
			sum2 += max;
		}
	auto pval = (sum1*lamda1+sum2*lamda2)/((h - 2)*(w - 2));
	std::cout << "P = "<< pval << std::endl;
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
//#define STD_UVC

void main()
{
	FrameDemo demo;
	TopGear::DeepCamAPI::Initialize();
	//TopGear::DeepCamAPI::EnableLog(TopGear::LogType::Standard | TopGear::LogType::DailyFile);
	TopGear::DeepCamAPI::SetLogLevel(TopGear::Level::Info);
	TopGear::DeepCamAPI::WriteLog(TopGear::Level::Info, "Initialized");
#ifdef STD_UVC
	//auto uvcDevices = deepcam.EnumerateDevices(TopGear::DeviceType::Standard);
	std::shared_ptr<TopGear::IVideoStream> uvc;
	//for (auto dev : uvcDevices)
	//{
	//	std::cout << dev->GetFriendlyName() << std::endl;
	//	std::cout << dev->GetSymbolicLink() << std::endl;
		uvc = TopGear::DeepCamAPI::CreateCamera(TopGear::Camera::StandardUVC);
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
		camaro = TopGear::DeepCamAPI::CreateCamera(TopGear::Camera::Camaro);
		auto ioControl = TopGear::DeepCamAPI::QueryInterface<TopGear::IDeviceControl>(camaro);
		auto cameraControl = TopGear::DeepCamAPI::QueryInterface<TopGear::ICameraControl>(camaro);
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
			TopGear::PropertyData<uint8_t> role;
			TopGear::PropertyData<std::string> info;
			ioControl->GetControl("DeviceInfo", info);
			std::cout << info.Payload << std::endl;
			if (ioControl->GetControl("DeviceRole", role) && role.Payload == 0)
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
	auto dual = TopGear::DeepCamAPI::CreateCamera(TopGear::Camera::CamaroDual);
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





