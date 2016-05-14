#pragma once
#include <string>
#include <functional>
#include <thread>
#include <map>
#include <mutex>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "LSource.h"
#include "IMultiVideoSource.h"

//#define USE_V4L2_USER_POINTER
//#define USE_CUDA_UNIFIED_MEMORY
//#define COPY_TO_USER
//#define USE_SINGLE_STREAM_LOCK

namespace TopGear
{
    namespace Linux
	{
        class VideoSourceReader : public IMultiVideoSource
		{
		public:
#ifdef USE_SINGLE_STREAM_LOCK
        	static std::mutex mtx;
#endif
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
            static const int BUFFER_SIZE = 8;
            static const int FRAMEQUEUE_SIZE = 20;

            struct StreamState
            {
                std::vector<VideoFormat> formats;
                ReaderCallbackFn fncb = nullptr;
                long defaultStride = 0;
                int currentFormatIndex = 0;
                bool isRunning = false;
                bool streamOn = false;
                std::thread streamThread;
                uint32_t imageSize = 0;
                uint8_t *mbuffers[BUFFER_SIZE];   //Mapping mmap memory
#ifdef COPY_TO_USER
                uint8_t *vbuffers[FRAMEQUEUE_SIZE]; //User memory buffer
                std::pair<std::weak_ptr<IVideoFrame>, bool> framesRef[FRAMEQUEUE_SIZE];
#endif
                uint64_t frameCounter = 0;
            };

            std::map<uint32_t, StreamState> streams;
            std::shared_ptr<IGenericVCDevice> deviceBackup;

            void Initmmap(uint32_t handle);
            void Uninitmmap(uint32_t handle);
            std::shared_ptr<IVideoFrame> RequestFrame(int handle, int &index);
            bool ReleaseFrame(int handle, int index);
            void OnReadWorker(uint32_t handle);
		};
	}
}
