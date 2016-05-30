#include "FlyCaptureReader.h"
#include "FlyCaptureSource.h"
#include "VideoSourceProxy.h"
#include "VideoBufferLock.h"

#include <functional>
#include <array>
#include <cstring>

using namespace TopGear;

#ifdef __linux__
#include "v4l2helper.h"
using namespace Linux;
#endif

#ifdef USE_CUDA
#include "cuda_runtime.h"
#endif

std::shared_ptr<IVideoStream> FlyCaptureReader::CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device,
                                                                  std::shared_ptr<bool> &vflip)
{
    auto player = std::make_shared<FlyCaptureReader>(device, vflip);
    return std::make_shared<VideoSourceProxy>(
                std::static_pointer_cast<IMultiVideoSource>(player), 0);
}

FlyCaptureReader::FlyCaptureReader(std::shared_ptr<IGenericVCDevice> &device,
                                   std::shared_ptr<bool> &vflip)
    : flipState(vflip), frameCounter(0), deviceBackup(device)
{
    auto source = std::dynamic_pointer_cast<FlyCaptureSource>(device->GetSource());
    if (source == nullptr)
        return;
    pCamera = &source->GetCameraSource();

    // Set the camera configuration
    FlyCapture2::FC2Config conf;
    auto error = pCamera->GetConfiguration(&conf);
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


    //Set default saturation 120%
    FlyCapture2::Property prop;
    prop.type = FlyCapture2::SATURATION;
    prop.onOff = true;
    prop.onePush = false;
    prop.autoManualMode = false;
    prop.absControl = true;
    prop.absValue = 120.0f;
    pCamera->SetProperty(&prop);

    EnumerateFormats();

    //Clear
    for(auto i = 0; i < BUFFER_SIZE; ++i)
    {
        mbuffers[i] = nullptr;
        framesLock[i] = false;
        framesReset[i] = false;
    }

    workerThread = std::thread(&FlyCaptureReader::RecycleWorker, this);
}

FlyCaptureReader::~FlyCaptureReader()
{
    StopStream(0xffffffffu);    //Force releasing buffers
    isRunning = false;
    if (workerThread.joinable())
        workerThread.join();
}

void FlyCaptureReader::EnumerateFormats()
{
    bool supported;
    FlyCapture2::Format7Info info;
    pCamera->GetFormat7Info(&info, &supported);
    if (!supported)
        return;
    VideoFormat format;
    std::memcpy(format.PixelFormat, "UYVY", 4);
    std::array<std::pair<uint32_t,uint32_t>, 3> resArray {
        std::make_pair(1920u,1080u),
        std::make_pair(1280u,720u),
        std::make_pair(800u,480u) };

    for(auto res : resArray)
        if (info.maxWidth>=res.first && info.maxHeight>=res.second)
        {
            format.Width = res.first;
            format.Height = res.second;
            format.MaxRate = 30;
            formats.push_back(format);
            format.MaxRate = 25;
            formats.push_back(format);
            format.MaxRate = 20;
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

    bool valid;
    FlyCapture2::Format7PacketInfo packetinfo;
    pCamera->ValidateFormat7Settings(&setting,&valid,&packetinfo);
    if (!valid)
        return false;

    psz = packetinfo.recommendedBytesPerPacket;
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
    prop.onOff = true;
    prop.onePush = false;
    prop.autoManualMode = false;
    prop.absControl = true;
    prop.absValue = formats[formatIndex].MaxRate;
    pCamera->SetProperty(&prop);

    defaultStride = formats[formatIndex].Width*2;

    return true;
}

void FlyCaptureCallback(FlyCapture2::Image* pImage, const void* pCallbackData)
{
    auto reader = (FlyCaptureReader *)pCallbackData;
    reader->OnReadSample(pImage, nullptr);
}

bool FlyCaptureReader::StartStream(uint32_t index)
{
    if (pCamera == nullptr || index!=0)
        return false;
    if (streamOn && !StopStream(index))
        return false;

    int imageSize = defaultStride * formats[currentFormatIndex].Height;

    for(auto i = 0; i < BUFFER_SIZE; ++i)
    {
        if (framesLock[i])
        {
            framesReset[i] = true;
            continue;
        }
#ifdef USE_CUDA
        if (cudaMallocHost(&mbuffers[i], imageSize,
                           cudaHostAllocPortable|cudaHostAllocMapped)!=cudaSuccess)
            throw std::runtime_error("cudaMallocHost failed!");
#else
        mbuffers[i]= new uint8_t[imageSize];
#endif
    }

    auto bufferSize = (imageSize+1023)/1024*1024;
    vbuffer = std::unique_ptr<uint8_t[]>(new uint8_t[bufferSize]);
    pCamera->SetUserBuffers(vbuffer.get(), bufferSize, 1);

    auto error = pCamera->StartCapture(FlyCaptureCallback, this);
    streamOn = error==FlyCapture2::PGRERROR_OK;
    return streamOn;
}

bool FlyCaptureReader::StopStream(uint32_t index)
{
    if (pCamera == nullptr)
        return false;
    auto error = pCamera->StopCapture();
    if (error==FlyCapture2::PGRERROR_OK)
    {
        for(auto i = 0; i < BUFFER_SIZE; ++i)
        {
            if (mbuffers[i] == nullptr)
                continue;
            if (framesLock[i] && index!=0xffffffffu)
            {
                framesReset[i] = true;
                continue;
            }
#ifdef USE_CUDA
            cudaFreeHost(mbuffers[i]);
#else
            delete[] mbuffers[i];
#endif
            mbuffers[i] = nullptr;

        }
        streamOn = false;
    }
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
    std::unique_lock<std::mutex> lk(mtx, std::try_to_lock);

    if (!lk.owns_lock())
    {
        ++frameCounter;
        return;
    }

    //Find available buffer slot
    int index = 0;
    while(index<BUFFER_SIZE)
    {
        if (!framesLock[index])
            break;
        ++index;
    }
    uint32_t counter = frameCounter++;
    if (index>=BUFFER_SIZE) //All taken
        return;
    if (*flipState)
    {
        uint8_t *src = pImage->GetData() + (pImage->GetDataSize()-pImage->GetStride());
        uint8_t *dst = mbuffers[index];
        for(int y=pImage->GetRows()-1; y>=0; --y)
        {
#ifdef __ARM_NEON__
            neonMemCopy_gas(dst, src, pImage->GetStride());
#else
            memcpy(dst, src, pImage->GetStride());
#endif
            src-=pImage->GetStride();
            dst+=pImage->GetStride();
        }
    }
    else
#ifdef __ARM_NEON__
        neonMemCopy_gas(mbuffers[index], pImage->GetData(), pImage->GetDataSize());
#else
        memcpy(mbuffers[index], pImage->GetData(), pImage->GetDataSize());
#endif

    auto tm = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch());

    std::shared_ptr<IVideoFrame> frame(new VideoBufferLock(
                                           0,
                                           index,
                                           mbuffers[index],
                                           tm.count(),
                                           counter,
                                           defaultStride,
                                           formats[currentFormatIndex],
                                           pImage->GetDataSize()));
    framesRef[index] = frame;
    framesLock[index] = true;

    if (fncb)
        fncb(frame);
}

void FlyCaptureReader::RecycleWorker()
{
    while(isRunning)
    {
        //Release unused frames
        for(int i=0;i<BUFFER_SIZE;++i)
            if (framesLock[i] & framesRef[i].expired())
            {
                if (framesReset[i])
                {
#ifdef USE_CUDA
                    cudaFreeHost(mbuffers[i]);
#else
                    delete[] mbuffers[i];
#endif
                    if (streamOn)
                    {
                        int imageSize = defaultStride * formats[currentFormatIndex].Height;
#ifdef USE_CUDA
                        if (cudaMallocHost(&mbuffers[i], imageSize,
                           cudaHostAllocPortable|cudaHostAllocMapped)!=cudaSuccess)
                            throw std::runtime_error("cudaMallocHost failed!");
#else
                        mbuffers[i]= new uint8_t[imageSize];
#endif
                    }
                    else
                        mbuffers[i] = nullptr;
                    framesReset[i] = false;
                }
                framesLock[i] = false;
            }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
