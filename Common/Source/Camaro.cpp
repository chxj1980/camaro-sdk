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

void Camaro::PostProcess(std::vector<IVideoFramePtr> &frames)
{
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
}

int Camaro::Flip(bool vertical, bool horizontal)
{
	if (registerMap==nullptr || sensorInfo!="DGFA")
		return -1;
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

int Camaro::GetExposure(bool &ae, float &ev)
{
    ev = 1.0f;
    auto result = -1;
    try
    {
        PropertyData<uint8_t> data;
        if (GetControl("AutoExposure", data))
        {
            ae = data.Payload;
            result = 0;
        }
    }
    catch (const std::out_of_range&)
    {
    }
    return result;
}

int Camaro::SetExposure(bool ae, float ev)
{
    (void)ev;
    auto result = -1;
    try
    {
        if (SetControl("AutoExposure", PropertyData<uint8_t>(ae?1:0)))
            result = 0;
    }
    catch (const std::out_of_range&)
    {
    }
    return result;
}

//////////////////////////////////////////////////
//    aptina specific
//
///////////////////////////////////////////////////

/*
* exposure time : val (multiple of 33.33us)
*/

int Camaro::GetShutter(uint32_t& val)
{
	if (registerMap==nullptr || sensorInfo!="DGFA")
		return -1;
	auto result = -1;
	try
	{
        auto addrs = registerMap->at("Shutter").AddressArray;
		if (addrs.size() != 1)
			return result;
        uint16_t data;
        result = GetRegister(addrs[0], data);	//AR0134_RR_D P.17
        val = data*100/3;
	}
	catch (const std::out_of_range&)
	{
	}
	//return GetRegister(0x3012, val);
	return result;
}

int Camaro::SetShutter(uint32_t val)
{
	if (registerMap==nullptr || sensorInfo!="DGFA")
		return -1;
	auto result = -1;
	try
	{
        bool ae;
        float ev;
        auto hr = GetExposure(ae, ev);
        if (hr>=0 && ae)    //AE enable
        {
            if (SetControl("ShutterLimit", PropertyData<uint32_t>(val)))
                result = 0;
        }
        else    //Manual Exposure
        {
            auto addrs = registerMap->at("Shutter").AddressArray;
            auto data = uint16_t(val * 3.0f/100 + 0.5f);
            if (data == 0)
                data = 1;
            if (addrs.size() != 1)
                return result;
            result = SetRegister(addrs[0], val);
        }
	}
	catch (const std::out_of_range&)
	{
	}
	//return SetRegister(0x3012, val);
	return result;
}

/*
gain:   xxx.yyyyy
default : 001.00000  (1x)
the step size for yyyyy is 0.03125(1/32), while the step size of xxx is 1.
*/
int Camaro::GetGain(float& gainR, float& gainG, float& gainB)
{
	if (registerMap==nullptr || sensorInfo!="DGFA")
		return -1;
	auto result = -1;
	uint16_t val[4];
	try
	{
		auto addrs = registerMap->at("Gain").AddressArray;
		if (addrs.size() != 4)
			return result;
        result = GetRegisters(addrs.data(), val, 4);
        gainR = (val[2]>>5)+(val[2]&0x1f)*0.03125f;
        gainB = (val[3]>>5)+(val[3]&0x1f)*0.03125f;
        gainG = (val[0]>>5)+(val[0]&0x1f)*0.03125f;
		//gainGb = val[1];
	}
	catch (const std::out_of_range&)
	{
	}
	//uint16_t regaddr[4]{ 0x3056, 0x305C, 0x305A, 0x3058 }; //AR0134 DG page 4, Gr/Gb/R/B
	//auto r = GetRegisters(regaddr, val, 4);
	return result;
}

int Camaro::SetGain(float gainR, float gainG, float gainB)
{
	if (registerMap==nullptr || sensorInfo!="DGFA")
		return -1;
	//uint16_t regaddr[4]{ 0x3056,0x305C, 0x305A, 0x3058 };//AR0134 DG page 4
    uint16_t r = (int(gainR)<<5);
    gainR -= r;
    r |= int(gainR/0.03125f)&0x1f;

    uint16_t g = (int(gainG)<<5);
    gainG -= g;
    r |= int(gainG/0.03125f)&0x1f;

    uint16_t b = (int(gainB)<<5);
    gainB -= b;
    b |= int(gainB/0.03125f)&0x1f;

    uint16_t val[4] { g, g, r, b };
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
	{
        auto query = config.QueryRegisterMap(info.Payload);
        sensorInfo = query.first;
        registerMap = query.second;
    }
	ObtainExtendedLines();
    formats = pReader->GetAllFormats();
	if (header != 0 || footer != 0)
		for (auto &f : formats)
			f.Height -= header + footer;

    Flip(false, false);
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
        return VideoFormat::Null;
	return formats[currentFormatIndex];
}
bool Camaro::SetCurrentFormat(uint32_t formatIndex)
{
	if (!pReader->SetCurrentFormat(formatIndex))
		return false;
	currentFormatIndex = formatIndex;
	return true;
}

bool Camaro::StartStream()
{
	if (currentFormatIndex < 0)
		return false;

	SetControl("Trigger", PropertyData<uint8_t>(0));
    SetControl("Resync", PropertyData<uint16_t>(900));

    //Flip(true, false);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	if (CameraSoloBase::StartStream())
	{
		SetControl("Trigger", PropertyData<uint8_t>(1));
		return true;
	}
	return false;
}

bool Camaro::StopStream()
{
	auto result = CameraSoloBase::StopStream();
	if (result)
		SetControl("Trigger", PropertyData<uint8_t>(0));
	return result;
}
