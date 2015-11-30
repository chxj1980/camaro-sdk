#include "Camaro.h"
#include <thread>
#include <array>
#include "VideoFrameEx.h"

using namespace TopGear;

int Camaro::SetRegisters(uint16_t regaddr[], uint16_t regval[], int num)
{
	if (config.RegisterCode == 0)
		return false;
	std::array<uint8_t, 50> data;
	data[0] = 1;  //write registers
	data[1] = num;
	for (auto i = 0; i < num; i++)
	{
		data[2 + (i << 1)] = regaddr[i] >> 8;    //high addr first
		data[3 + (i << 1)] = regaddr[i] & 0xff;  //low addr
		data[26 + (i << 1)] = regval[i] >> 8;    //high data
		data[27 + (i << 1)] = regval[i] & 0xff;    //low data
	}
	return extensionAdapter.Source().SetProperty(config.RegisterCode, data);
}

int Camaro::GetRegisters(uint16_t regaddr[], uint16_t regval[], int num)
{
	if (config.RegisterCode == 0)
		return false;
	std::array<uint8_t, 50> prop;
	prop[0] = 0;  //read registers
	prop[1] = num;

	for (auto i = 0; i < num; i++)
	{
		prop[2 + (i << 1)] = regaddr[i] >> 8;    //high addr first
		prop[3 + (i << 1)] = regaddr[i] & 0xff;  //low addr
	}

	auto res = extensionAdapter.Source().SetProperty(config.RegisterCode, prop);
	if (res)
		return res;
	auto len = 0;
	auto data = extensionAdapter.Source().GetProperty(config.RegisterCode, len);

	if (data == nullptr || len<(3+(num<<1)))
		return -1;

	for (auto i = 0; i < num; i++)
		regval[i] = (data[2 + (i << 1)] << 8) | data[3 + (i << 1)];
	return 0;
}

inline int Camaro::SetRegister(uint16_t regaddr, uint16_t regval)
{
	return SetRegisters(&regaddr, &regval, 1);
}

inline int Camaro::GetRegister(uint16_t regaddr, uint16_t &regval)
{
	return GetRegisters(&regaddr, &regval, 1);
}

void Camaro::ObtainExtendedLines()
{
	if (registerMap == nullptr)
		return;
	uint16_t val = 0;
	try 
	{
		auto addrs = registerMap->at("EmbeddedData").AddressArray;
		if (addrs.size() != 1)
			return;
		GetRegister(addrs[0], val);	//AR0134_RR_D P.17
	}
	catch (const std::out_of_range&) 
	{
		header = 0;
		footer = 0;
		return;
		//std::cerr << "Out of Range error: " << oor.what() << '\n';
	}
	
	//GetRegister(0x3064, val);	//AR0134_RR_D P.17
	auto enH = (val & (1 << 8));
	auto enF = enH ? val & (1 << 7) : false;
	header = enH ? EMBEDDED_LINES : 0;
	footer = enF ? EMBEDDED_LINES : 0;
}

void Camaro::OnFrame(IVideoStream &parent, std::vector<IVideoFramePtr>& frames)
{
    (void)parent;
	if (frames.size() != 1)
		return;
	auto frame = frames[0];
	
	uint32_t w = formats[currentFormatIndex].Width;
	uint32_t h = formats[currentFormatIndex].Height;

	uint8_t *pData;
	uint32_t stride;
	if (frame->LockBuffer(&pData, &stride) != 0)
		return;
	uint16_t index = (pData[1] & 0xf0) << 8 | (pData[3] & 0xf0) << 4 |
		(pData[5] & 0xf0) | (pData[7] & 0xf0) >> 4;
	frame->UnlockBuffer();

	IVideoFramePtr ex = std::make_shared<VideoFrameEx>(frame, header*stride, stride, w, h, index,
		(header + h)*stride, footer > 0 ? footer*stride : 0);
	frames.clear();
	frames.emplace_back(ex);
	if (fnCb)
		fnCb(*this, frames);
}

int Camaro::Flip(bool vertical, bool horizontal)
{
	uint16_t val = 0;
	if (vertical)
		val |= (1 << 15);
	if (horizontal)
		val |= (1 << 14);
	auto result = -1;
	try
	{
		auto addrs = registerMap->at("Flip").AddressArray;
		if (addrs.size() != 1)
			return result;
		result = SetRegister(addrs[0], val);
	}
	catch (const std::out_of_range&)
	{
	}
	//return SetRegister(0x3040, val);
	return result;
}

bool Camaro::SetControl(std::string name, IPropertyData &&val)
{
	auto& obj = val;
	return SetControl(name, obj);
}

bool Camaro::SetControl(std::string name, IPropertyData &val)
{
	if (&config == &CameraProfile::NullObject())
		return false;
	auto it = config.XuControls.find(name);
	if (it != config.XuControls.end())
	{
		if (it->second.TypeHash != val.GetTypeHash())
			return false;
		if (it->second.Attribute.find('w') == std::string::npos)
			return false;
		//Find fixed prefix
		auto pit = it->second.Payloads.find("w");
		size_t prefixLen = 0;
		uint8_t *pPrefix = nullptr;
		if (pit != it->second.Payloads.end())
		{
			pPrefix = pit->second.second.data();
			prefixLen = pit->second.second.size();
		}

		if (val.GetTypeHash() == typeid(uint8_t).hash_code())
			return extensionAdapter.SetProperty<uint8_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(uint16_t).hash_code())
			return extensionAdapter.SetProperty<uint16_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(int).hash_code())
			return extensionAdapter.SetProperty<int32_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(std::string).hash_code())
			return extensionAdapter.SetProperty<std::string>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(std::vector<uint8_t>).hash_code())
			return extensionAdapter.SetProperty<std::vector<uint8_t>>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
	}
	return false;
}
bool Camaro::GetControl(std::string name, IPropertyData &val)
{
	if (&config == &CameraProfile::NullObject())
		return false;
	auto it = config.XuControls.find(name);
	if (it != config.XuControls.end())
	{
		if (it->second.TypeHash != val.GetTypeHash())
			return false;
		if (it->second.Attribute.find('r') == std::string::npos)
			return false;
		//Find fixed prefix
		auto pit = it->second.Payloads.find("r");
		size_t prefixLen = 0;
		uint8_t *pPrefix = nullptr;
		if (pit != it->second.Payloads.end())
		{
			if (pit->second.first)	//Need verify prefix
				pPrefix = pit->second.second.data();
			prefixLen = pit->second.second.size();
		}

		if (val.GetTypeHash() == typeid(uint8_t).hash_code())
			return extensionAdapter.GetProperty<uint8_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(uint16_t).hash_code())
			return extensionAdapter.GetProperty<uint16_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(int).hash_code())
			return extensionAdapter.GetProperty<int32_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(std::string).hash_code())
			return extensionAdapter.GetProperty<std::string>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
		if (val.GetTypeHash() == typeid(std::vector<uint8_t>).hash_code())
			return extensionAdapter.GetProperty<std::vector<uint8_t>>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
	}
	return false;
}

//int Camaro::SetSensorTrigger(uint8_t level)
//{
//	return extension->SetProperty(static_cast<int>(ControlCode::Trigger), level);
//}
//
//int Camaro::SetResyncNumber(uint16_t resyncNum)
//{
//	std::array<uint8_t, 3> data;
//	data[0] = 1;  //write registers
//	data[1] = resyncNum >> 8;
//	data[2] = resyncNum & 0xff;
//	return extension->SetProperty(static_cast<int>(ControlCode::Resync), data);
//}
//
//int Camaro::QueryDeviceRole()
//{
//	auto len = 0;
//	auto data = extension->GetProperty(static_cast<int>(ControlCode::DeviceRole), len);
//	if (data==nullptr || len<2)
//		return -1;
//	return data[1];
//}
//
//std::string Camaro::QueryDeviceInfo()
//{
//	auto len = 0;
//	auto data = extension->GetProperty(static_cast<int>(ControlCode::DeviceInfo), len);
//
//	if (data == nullptr)
//	{
//		//std::cout<<"Unable to get property value\n";
//		return "";
//	}
//	std::string info(reinterpret_cast<char *>(data.get()));
//	return info;
//}

//////////////////////////////////////////////////
//    aptina specific
//
///////////////////////////////////////////////////

/*
* exposure time = val * 33.33us
*/

inline int Camaro::GetExposure(uint16_t& val)
{
	auto result = -1;
	try
	{
		auto addrs = registerMap->at("Exposure").AddressArray;
		if (addrs.size() != 1)
			return result;
		result = GetRegister(addrs[0], val);	//AR0134_RR_D P.17
	}
	catch (const std::out_of_range&)
	{
	}
	//return GetRegister(0x3012, val);
	return result;
}

inline int Camaro::SetExposure(uint16_t val)
{
	auto result = -1;
	try
	{
		auto addrs = registerMap->at("Exposure").AddressArray;
		if (addrs.size() != 1)
			return result;
		result = SetRegister(addrs[0], val);
	}
	catch (const std::out_of_range&)
	{
	}
	//return SetRegister(0x3012, val);
	return result;
}

/*
gain:   xxx.yyyyy
default : 001.000000  (1x)
the step size for yyyyy is 0.03125(1/32), while the step size of xxx is 1.
*/
int Camaro::GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB)
{
	auto result = -1;
	uint16_t val[4];
	try
	{
		auto addrs = registerMap->at("Gain").AddressArray;
		if (addrs.size() != 4)
			return result;
		result = GetRegisters(addrs.data(), val, 4);
		gainR = val[2];
		gainB = val[3];
		gainG = val[0];
		//gainGb = val[1];
	}
	catch (const std::out_of_range&)
	{
	}
	//uint16_t regaddr[4]{ 0x3056, 0x305C, 0x305A, 0x3058 }; //AR0134 DG page 4, Gr/Gb/R/B
	//auto r = GetRegisters(regaddr, val, 4);
	return result;
}

int Camaro::SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB)
{
	//uint16_t regaddr[4]{ 0x3056,0x305C, 0x305A, 0x3058 };//AR0134 DG page 4
	uint16_t val[4]{ gainG, gainG, gainR, gainB };
	//return SetRegisters(regaddr, val, 4);
	auto result = -1;
	try
	{
		auto addrs = registerMap->at("Gain").AddressArray;
		if (addrs.size() != 4)
			return result;
		result = SetRegisters(addrs.data(), val, 4);
	}
	catch (const std::out_of_range&)
	{
	}
	return result;
}

Camaro::Camaro(std::shared_ptr<IVideoStream>& vs, 
	std::shared_ptr<IExtensionAccess>& ex,
	CameraProfile &con)
	: CameraSoloBase(vs, con), extensionAdapter(ex)
{
	PropertyData<std::string> info;
	if (Camaro::GetControl("DeviceInfo", info))
		registerMap = config.QueryRegisterMap(info.Payload);

	vs->RegisterFrameCallback(std::bind(&Camaro::OnFrame, this, std::placeholders::_1, std::placeholders::_2));
	ObtainExtendedLines();
    formats = pReader->GetAllFormats();
	if (header != 0 || footer != 0)
		for (auto &f : formats)
			f.Height -= header + footer;
}

Camaro::~Camaro()
{
	Camaro::StopStream();
}

int Camaro::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
	auto hr = pReader->GetOptimizedFormatIndex(format, fourcc);
	if (header == 0 && footer == 0)
		return hr;
	format.Height -= header + footer;
	return hr;
}

int Camaro::GetMatchedFormatIndex(const VideoFormat& format) const
{
	if ((header == 0 && footer == 0) || format.Height == 0)
		return pReader->GetMatchedFormatIndex(format);
	auto amended(format);
	amended.Height += header + footer;
	return pReader->GetMatchedFormatIndex(amended);
}

const std::vector<VideoFormat>& Camaro::GetAllFormats() const
{
	return formats;
}

const VideoFormat &Camaro::GetCurrentFormat() const
{
	if (currentFormatIndex < 0)
		return VideoFormatNull;
	return formats[currentFormatIndex];
}
bool Camaro::SetCurrentFormat(uint32_t formatIndex)
{
	if (!pReader->SetCurrentFormat(formatIndex))
		return false;
	currentFormatIndex = formatIndex;
	return true;
}

void Camaro::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fnCb = fn;
}

void Camaro::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	fnCb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

bool Camaro::StartStream()
{
	if (currentFormatIndex < 0)
		return false;

	SetControl("Trigger", PropertyData<uint8_t>(0));
	SetControl("Resync", PropertyData<uint16_t>(100));
	//SetSensorTrigger(0);
	//SetResyncNumber(900);
	Flip(true, false);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	if (CameraSoloBase::StartStream())
	{
		SetControl("Trigger", PropertyData<uint8_t>(1));
//		SetSensorTrigger(1);
		return true;
	}
	return false;
}

bool Camaro::StopStream()
{
	CameraSoloBase::StopStream();
	SetControl("Trigger", PropertyData<uint8_t>(0));
	//SetSensorTrigger(0);
	return true;
}
