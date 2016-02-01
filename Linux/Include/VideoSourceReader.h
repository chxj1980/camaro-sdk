#pragma once
#include <string>
#include <functional>
#include <thread>
#include <map>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "LSource.h"
#include "IMultiVideoSource.h"


namespace TopGear
{
    namespace Linux
	{
        class VideoSourceReader : public IMultiVideoSource
		{
		public:
            static std::vector<std::shared_ptr<IVideoStream>> CreateVideoStreams(std::shared_ptr<IGenericVCDevice> &device);
            static std::shared_ptr<IVideoStream> CreateVideoStream(std::shared_ptr<IGenericVCDevice> &device);

            virtual const std::vector<VideoFormat> &GetAllFormats(uint32_t handle) override;
            virtual bool SetCurrentFormat(uint32_t handle, int formatIndex) override;
            virtual bool StartStream(uint32_t handle) override;
            virtual bool StopStream(uint32_t handle) override;
            virtual bool IsStreaming(uint32_t handle) override;
            virtual void RegisterReaderCallback(uint32_t handle, const ReaderCallbackFn& fn) override;
            virtual ~VideoSourceReader();
		protected:
            VideoSourceReader(std::shared_ptr<IGenericVCDevice> &device);
            void EnumerateStreams(const LSource &one);
            void EnumerateFormats(uint32_t handle);
        private:
            static const int FRAMEQUEUE_SIZE = 4;

            struct StreamState
            {
                std::vector<VideoFormat> formats;
                ReaderCallbackFn fncb = nullptr;
                long defaultStride = 0;
                int currentFormatIndex = 0;
                bool isRunning = false;
                bool streamOn = false;
                std::thread streamThread;
                std::pair<uint8_t *,int> vbuffers[FRAMEQUEUE_SIZE];
            };

            std::map<uint32_t, StreamState> streams;
            std::shared_ptr<IGenericVCDevice> deviceBackup;

            void Initmmap(uint32_t handle);
            void Uninitmmap(uint32_t handle);
            std::shared_ptr<IVideoFrame> RequestFrame(int index, int &mindex);
            bool ReleaseFrame(int index, int mindex);
            void OnReadWorker(uint32_t handle);
		};
	}
}
