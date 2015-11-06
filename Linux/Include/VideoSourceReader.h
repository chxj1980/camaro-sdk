#pragma once
#include <string>
#include <functional>
#include <thread>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "ILSource.h"


namespace TopGear
{
    namespace Linux
	{
        class VideoSourceReader : public IVideoStream
		{
		public:
			virtual int GetOptimizedFormatIndex(VideoFormat &format, const char *fourcc = "") override;
			virtual int GetMatchedFormatIndex(const VideoFormat &format) const override;

			virtual const std::vector<VideoFormat> &GetAllFormats() const override
			{ return videoFormats; }


            virtual const VideoFormat& GetCurrentFormat() const override
            {
                return videoFormats[currentFormatIndex];
            }
			
			virtual bool StartStream(int formatIndex) override;
			virtual bool StopStream() override;
			virtual bool IsStreaming() const override
			{ return streamOn; }
			virtual void RegisterFrameCallback(IVideoFrameCallback *pCB) override;
			virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
            VideoSourceReader(std::shared_ptr<ILSource> &source);
            virtual ~VideoSourceReader();
		protected:
			VideoFrameCallbackFn fnCb = nullptr;
			IVideoFrameCallback *pCbobj = nullptr;
			std::vector<VideoFormat> videoFormats;
            std::shared_ptr<ILSource> handleSource;

            //bool IsFormatSupported(const GUID &subtype) const;
			void EnumerateFormats();

            int handle;
			bool isRunning = false;
			bool streamOn = false;
			long defaultStride = 0;
			int frameWidth = 0;
			int frameHeight = 0;
        private:
            static const int FRAMEQUEUE_SIZE = 4;
            int currentFormatIndex = 0;
            std::thread streamThread;
            std::pair<uint8_t *,int> vbuffers[FRAMEQUEUE_SIZE];
            void Initmmap();
            void Uninitmmap();
            std::shared_ptr<IVideoFrame> RequestFrame(int &index);
            bool ReleaseFrame(int index);
            void OnReadWorker();
		};
	}
}
