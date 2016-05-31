#pragma once
#include <string>
#include <functional>
#include <thread>
#include <map>
#include <mutex>
#include <atomic>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "IMultiVideoSource.h"

#include <flycapture/FlyCapture2.h>

#include "sys_config.h"

namespace TopGear
{
    class FlyCaptureReader : public IMultiVideoSource
    {
    public:
        static std::shared_ptr<IVideoStream> CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device,
                                                               std::shared_ptr<bool> &vflip);

        virtual const std::vector<VideoFormat> &GetAllFormats(uint32_t index) override;
        virtual bool SetCurrentFormat(uint32_t index, int formatIndex) override;
        virtual bool StartStream(uint32_t index) override;
        virtual bool StopStream(uint32_t index) override;
        virtual bool IsStreaming(uint32_t index) override;
        virtual void RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn) override;
        explicit FlyCaptureReader(std::shared_ptr<IGenericVCDevice> &device,
                                  std::shared_ptr<bool> &vflip);
        virtual ~FlyCaptureReader();
        void OnReadSample(FlyCapture2::Image* pImage, const void* pCallbackData);

    protected:
        void EnumerateFormats();

    private:
        static const int BUFFER_SIZE = VIDEO_BUFFER_NUM;

        FlyCapture2::Camera *pCamera = nullptr;

        std::shared_ptr<bool> flipState;

        std::atomic_uint frameCounter;
        std::vector<VideoFormat> formats;
        ReaderCallbackFn fncb = nullptr;
        long defaultStride = 0;
        int currentFormatIndex = 0;

        std::unique_ptr<uint8_t[]> vbuffer;
        uint8_t *mbuffers[BUFFER_SIZE];
        std::weak_ptr<IVideoFrame> framesRef[BUFFER_SIZE];
        std::atomic_bool framesLock[BUFFER_SIZE];
        std::atomic_bool framesReset[BUFFER_SIZE];

        bool isRunning = true;
        bool streamOn = false;

        std::shared_ptr<IGenericVCDevice> deviceBackup;

        std::thread workerThread;
        std::mutex mtx;

        void RecycleWorker();
    };
}


