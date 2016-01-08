#pragma once
#include "IVideoStream.h"
#include "IMultiVideoSource.h"

namespace TopGear
{
	class VideoSourceProxy final 
		: public IVideoStream
	{
	public:
        VideoSourceProxy(std::shared_ptr<IMultiVideoSource> reader, uint32_t index);
		virtual ~VideoSourceProxy() {}

		virtual bool StartStream() override;
		virtual bool StopStream() override;
		virtual bool IsStreaming() const override;
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc) override;
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
		virtual bool SetCurrentFormat(uint32_t formatIndex) override;
		virtual const std::vector<VideoFormat>& GetAllFormats() const override
		{
			return formats;
		}
		virtual const VideoFormat& GetCurrentFormat() const override;
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;
	private:
		void OnFrame(IVideoFramePtr &frame);
		std::shared_ptr<IMultiVideoSource> client;
		uint32_t streamIndex;
		const std::vector<VideoFormat> &formats;
		int currentFormatIndex = -1;
		VideoFrameCallbackFn fncb = nullptr;
	};
}

