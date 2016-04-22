#pragma once
#include <string>
#include <functional>
#include <thread>
#include <map>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "IMultiVideoSource.h"

#include <flycapture/FlyCapture2.h>


namespace TopGear
{
    class FlyCaptureReader : public IMultiVideoSource
    {
    public:
        static std::shared_ptr<IVideoStream> CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device);

        virtual const std::vector<VideoFormat> &GetAllFormats(uint32_t index) override;
        virtual bool SetCurrentFormat(uint32_t index, int formatIndex) override;
        virtual bool StartStream(uint32_t index) override;
        virtual bool StopStream(uint32_t index) override;
        virtual bool IsStreaming(uint32_t index) override;
        virtual void RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn) override;
        explicit FlyCaptureReader(std::shared_ptr<IGenericVCDevice> &device);
        virtual ~FlyCaptureReader();
    protected:
        void EnumerateFormats();
        void OnReadSample(FlyCapture2::Image* pImage, const void* pCallbackData);
    private:
        static const int FRAMEQUEUE_SIZE = 4;

        FlyCapture2::Camera *pCamera = nullptr;
        FlyCapture2::Image* pImageBuffer = nullptr;

        std::vector<VideoFormat> formats;
        ReaderCallbackFn fncb = nullptr;
        long defaultStride = 0;
        int currentFormatIndex = 0;
        bool isRunning = false;
        bool streamOn = false;
        std::thread streamThread;
        //std::pair<uint8_t *,int> vbuffers[FRAMEQUEUE_SIZE];

        std::shared_ptr<IGenericVCDevice> deviceBackup;


        std::shared_ptr<IVideoFrame> RequestFrame(int index, int &mindex);
        bool ReleaseFrame(int index, int mindex);
        void OnReadWorker(uint32_t handle);
    };
}


