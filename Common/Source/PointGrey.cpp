#include "PointGrey.h"
#include <flycapture/FlyCapture2.h>

using namespace TopGear;

PointGrey::PointGrey(std::shared_ptr<IVideoStream> &vs, std::shared_ptr<ISource> &source,
                     std::shared_ptr<bool> &vflip)
    : CameraSoloBase(vs), flipState(vflip)
{
    pgsource = std::dynamic_pointer_cast<FlyCaptureSource>(source);
    Flip(false, false);
}

PointGrey::~PointGrey()
{
    StopStream();
}

int PointGrey::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
    return pReader->GetOptimizedFormatIndex(format, fourcc);
}

int PointGrey::GetMatchedFormatIndex(const VideoFormat& format) const
{
    return pReader->GetMatchedFormatIndex(format);
}

const std::vector<VideoFormat>& PointGrey::GetAllFormats() const
{
    return pReader->GetAllFormats();
}

const VideoFormat &PointGrey::GetCurrentFormat() const
{
    if (currentFormatIndex < 0)
        return VideoFormat::Null;
    return pReader->GetAllFormats()[currentFormatIndex];
}

bool PointGrey::SetCurrentFormat(uint32_t formatIndex)
{
    if (!pReader->SetCurrentFormat(formatIndex))
        return false;
    currentFormatIndex = formatIndex;
    return true;
}

int PointGrey::Flip(bool vertical, bool horizontal)
{
    *flipState = vertical;
    auto &pg = pgsource->GetCameraSource();
    pg.WriteRegister(0x1054, horizontal?0x80000001u:0x80000000u);
    return 0;
}

int PointGrey::GetExposure(bool &ae, float &ev)
{
    auto &pg = pgsource->GetCameraSource();
    FlyCapture2::Property prop;
    prop.type = FlyCapture2::AUTO_EXPOSURE;
    pg.GetProperty(&prop);
    ae = prop.autoManualMode;
    ev = prop.absValue;
    return 0;
}

int PointGrey::SetExposure(bool ae, float ev)
{
    auto &pg = pgsource->GetCameraSource();
    FlyCapture2::Property prop;
    prop.type = FlyCapture2::AUTO_EXPOSURE;
    prop.onOff = true;
    prop.onePush = false;
    prop.autoManualMode = ae;
    prop.absControl = true;
    prop.absValue = ev;
    pg.SetProperty(&prop);

    //enable auto shutter
    prop.type = FlyCapture2::SHUTTER;
    prop.onOff = true;
    prop.onePush = false;
    prop.autoManualMode = true;
    prop.absControl = true;
    prop.absValue = 1.0f;
    pg.SetProperty(&prop);

    return 0;
}

int PointGrey::GetShutter(uint32_t &val)
{
    auto &pg = pgsource->GetCameraSource();
    FlyCapture2::Property prop;
    prop.type = FlyCapture2::SHUTTER;
    pg.GetProperty(&prop);
    val = uint32_t(prop.absValue*1000);
    return 0;
}

int PointGrey::SetShutter(uint32_t val)
{
    auto &pg = pgsource->GetCameraSource();
    FlyCapture2::Property prop;

    //Decide auto shutter switch by auto exposure
    prop.type = FlyCapture2::AUTO_EXPOSURE;
    pg.GetProperty(&prop);
    auto autoShutter = prop.autoManualMode && val==0;

    prop.type = FlyCapture2::SHUTTER;
    prop.onOff = true;
    prop.onePush = false;
    prop.autoManualMode = autoShutter;
    prop.absControl = true;
    prop.absValue = val/1000.0f;
    pg.SetProperty(&prop);
    return 0;
}

int PointGrey::GetGain(float& gainR, float& gainG, float& gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return 0;
}

int PointGrey::SetGain(float gainR, float gainG, float gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return 0;
}
