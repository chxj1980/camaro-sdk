
#include "CamaroMovidius.h"
#include <thread>
#include <array>
#include <cmath>
#include "VideoFrameEx.h"

using namespace TopGear;

int CamaroMovidius::Flip(bool vertical, bool horizontal)
{
    return SetControl("Flip", PropertyData<uint8_t>((vertical?0x01:0x00)|(horizontal?0x02:0x00)));
}

bool CamaroMovidius::SetControl(std::string name, IPropertyData &&val)
{
    auto& obj = val;
    return SetControl(name, obj);
}

bool CamaroMovidius::SetControl(std::string name, IPropertyData &val)
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
bool CamaroMovidius::GetControl(std::string name, IPropertyData &val)
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

int CamaroMovidius::GetExposure(bool &ae, float &ev)
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

int CamaroMovidius::SetExposure(bool ae, float ev)
{
    (void)ev;
    auto result = -1;
    float val = ev*128;
    uint8_t ev_int = (val>255)?255:(val<2?2:val);
    try
    {
        if (SetControl("AutoExposure", PropertyData<uint8_t>(ae?ev_int:0)))
            result = 0;
    }
    catch (const std::out_of_range&)
    {
    }
    return result;
}

int CamaroMovidius::GetShutter(uint32_t& val)
{
    PropertyData<int32_t> result;
    if (!GetControl("ShutterLimit", result))
        return -1;
    val = uint32_t(result.Payload);
    return 0;
}

int CamaroMovidius::SetShutter(uint32_t val)
{
    auto result = -1;
    bool ae;
    float ev;
    auto hr = GetExposure(ae, ev);
    if (hr>=0 && ae)    //AE enable
    {
        if (SetControl("ShutterLimit", PropertyData<int32_t>(int32_t(val))))
            result = 0;
    }
    else    //Manual Exposure
    {
    }
    return result;
}

int CamaroMovidius::GetGain(float& gainR, float& gainG, float& gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return -1;
}

int CamaroMovidius::SetGain(float gainR, float gainG, float gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return -1;
}

CamaroMovidius::CamaroMovidius(std::shared_ptr<IVideoStream>& vs,
                               std::shared_ptr<IExtensionAccess>& ex,
                               CameraProfile &con)
    : CameraSoloBase(vs, con), extensionAdapter(ex)
{
    PropertyData<std::string> info;
    if (CamaroMovidius::GetControl("DeviceInfo", info))
    {
        auto query = config.QueryRegisterMap(info.Payload);
        sensorInfo = query.first;
    }
    auto builtinFormats = pReader->GetAllFormats();
    for(auto &item : builtinFormats)
    {
        for(auto rate=10;rate<=30;rate+=5)
        {
            VideoFormat format(item);
            format.MaxRate = rate;
            formats.emplace_back(format);
        }
    }
    Flip(false, false);
}

CamaroMovidius::~CamaroMovidius()
{
    StopStream();
}

int CamaroMovidius::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
    auto bandwidth = 0;
    auto index = -1;
    auto i = -1;
    for (auto f : formats)
    {
        ++i;
        if (std::strcmp(fourcc, "") != 0 && std::strncmp(fourcc, f.PixelFormat, 4) != 0)
            continue;
        if (f.Width*f.Height*f.MaxRate > bandwidth)
            index = i;
    }
    if (index >= 0)
        format = formats[index];
    return index;
}

int CamaroMovidius::GetMatchedFormatIndex(const VideoFormat& format) const
{
    auto index = -1;
    for (auto i : formats)
    {
        index++;
        if (format.Width > 0 && format.Width != i.Width)
            continue;
        if (format.Height> 0 && format.Height != i.Height)
            continue;
        if (format.MaxRate > 0 && format.MaxRate != i.MaxRate)
            continue;
        if (std::strcmp(format.PixelFormat, "") != 0 && std::strncmp(format.PixelFormat, i.PixelFormat, 4) != 0)
            continue;
        return index;
    }
    return -1;
}

const std::vector<VideoFormat>& CamaroMovidius::GetAllFormats() const
{
    return formats;
}

const VideoFormat &CamaroMovidius::GetCurrentFormat() const
{
    if (currentFormatIndex < 0)
        return VideoFormat::Null;
    return formats[currentFormatIndex];
}

bool CamaroMovidius::SetCurrentFormat(uint32_t formatIndex)
{
    auto &format = formats[formatIndex];
    VideoFormat builtin(format);
    builtin.MaxRate = 0;
    auto index = pReader->GetMatchedFormatIndex(builtin);
    if (index<0)
        return false;
    if (!pReader->SetCurrentFormat(index))
        return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); //Need to wait, Movidius bug
    if (!SetControl("MaxRate", PropertyData<uint8_t>(format.MaxRate)))
        return false;
    currentFormatIndex = formatIndex;
    return true;
}

