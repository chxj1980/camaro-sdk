#include "flycapturereader.h"
#include "flycapturesource.h"
#include "VideoSourceProxy.h"

#include <functional>
#include <array>
#include <cstring>

using namespace TopGear;

std::shared_ptr<IVideoStream> FlyCaptureReader::CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device)
{
    auto player = std::make_shared<FlyCaptureReader>(device);
    return std::make_shared<VideoSourceProxy>(
                std::static_pointer_cast<IMultiVideoSource>(player), 0);
}

FlyCaptureReader::FlyCaptureReader(std::shared_ptr<IGenericVCDevice> &device)
    : deviceBackup(device)
{
    auto source = std::dynamic_pointer_cast<FlyCaptureSource>(device->GetSource());
    if (source == nullptr)
        return;
    pCamera = &source->GetCameraSource();
    // Set sensor pixel format as YUV422:
    FlyCapture2::Format7ImageSettings setting;
    unsigned int psz;
    float per;
    auto error = pCamera->GetFormat7Configuration(&setting, &psz, &per);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        //return SENSOR_GET_STATUS_ERROR;
        return;
    }
    setting.mode = FlyCapture2::MODE_0;
    setting.pixelFormat = FlyCapture2::PIXEL_FORMAT_422YUV8;
    psz = 24960;
    error = pCamera->SetFormat7Configuration(&setting, psz);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        //return SENSOR_SET_STATUS_ERROR;
        return;
    }
    // Set the camera configuration
    FlyCapture2::FC2Config conf;
    error = pCamera->GetConfiguration(&conf);
    if (error != FlyCapture2::PGRERROR_OK) {
        //PrintError(error);
        //return SENSOR_GET_STATUS_ERROR;

        return;
    }
    conf.grabTimeout = 1000;	// in ms
    error = pCamera->SetConfiguration(&conf);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        //return SENSOR_SET_STATUS_ERROR;
        return;
    }
    EnumerateFormats();
}

FlyCaptureReader::~FlyCaptureReader()
{
    StopStream(0);
    if (pImageBuffer)
    {
        pImageBuffer->ReleaseBuffer();
        delete pImageBuffer;
    }
}

void FlyCaptureReader::EnumerateFormats()
{
    FlyCapture2::Format7Info info;
    bool supported;
    pCamera->GetFormat7Info(&info, &supported);
    if (!supported)
        return;
    VideoFormat format;
    std::memcpy(format.PixelFormat, "YUY2", 4);
    std::array<std::pair<uint32_t,uint32_t>, 3> resArray {
        std::make_pair(1920u,1080u),
        std::make_pair(1280u,720u),
        std::make_pair(853u,480u) };

    for(auto res : resArray)
        if (info.maxWidth>=res.first && info.maxHeight>=res.second)
        {
            format.Width = res.first;
            format.Height = res.second;
            format.MaxRate = 30;
            formats.push_back(format);
            format.MaxRate = 24;
            formats.push_back(format);
            format.MaxRate = 15;
            formats.push_back(format);
        }
}


const std::vector<VideoFormat> &FlyCaptureReader::GetAllFormats(uint32_t index)
{
    (void)index;
    return formats;
}

bool FlyCaptureReader::SetCurrentFormat(uint32_t index, int formatIndex)
{
    if (index!=0)
        return false;
    if (uint(formatIndex)>=formats.size())
        return false;

    FlyCapture2::Format7Info info;
    bool supported;
    pCamera->GetFormat7Info(&info, &supported);
    if (!supported)
        return false;

    FlyCapture2::Format7ImageSettings setting;
    unsigned int psz;
    float per;
    auto error = pCamera->GetFormat7Configuration(&setting, &psz, &per);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        //return SENSOR_GET_STATUS_ERROR;
        return false;
    }
    setting.mode = FlyCapture2::MODE_0;
    setting.pixelFormat = FlyCapture2::PIXEL_FORMAT_422YUV8;
    setting.width = formats[formatIndex].Width;
    setting.height = formats[formatIndex].Height;
    setting.offsetX = (info.maxWidth - setting.width)/2;
    setting.offsetY = (info.maxHeight - setting.height)/2;
    psz = 24960;
    error = pCamera->SetFormat7Configuration(&setting, psz);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        //return SENSOR_SET_STATUS_ERROR;
        return false;
    }

    //Set frame rate
    FlyCapture2::Property prop;
    prop.type = FlyCapture2::FRAME_RATE;
    prop.onOff = 1;
    prop.onePush = 0;
    prop.autoManualMode = false;
    prop.absControl = 1;
    prop.absValue = formats[formatIndex].MaxRate;
    pCamera->SetProperty(&prop);

    return true;
}

bool FlyCaptureReader::StartStream(uint32_t index)
{
    if (pCamera == nullptr || index!=0)
        return false;
    std::function<void(FlyCapture2::Image*,const void*)> cb =
            std::bind(&FlyCaptureReader::OnReadSample, this,
                      std::placeholders::_1, std::placeholders::_2);
    auto error = pCamera->StartCapture(cb.target<void(FlyCapture2::Image*, const void*)>());
    streamOn = error==FlyCapture2::PGRERROR_OK;
    return streamOn;
}

bool FlyCaptureReader::StopStream(uint32_t index)
{
    if (pCamera == nullptr || index!=0)
        return false;
    auto error = pCamera->StopCapture();
    streamOn = false;
    return error==FlyCapture2::PGRERROR_OK;
}

bool FlyCaptureReader::IsStreaming(uint32_t index)
{
    if (index!=0)
        return false;
    return streamOn;
}

void FlyCaptureReader::RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn)
{
    if (index!=0)
        return;
    fncb = fn;
}

void FlyCaptureReader::OnReadSample(FlyCapture2::Image* pImage, const void* pCallbackData)
{
    (void)pCallbackData;
    pImageBuffer = pImage;
//    if (fncb)
//    {
//        VideoBufferLock
//        fncb(pImageBuffer, pCallbackData);
}
