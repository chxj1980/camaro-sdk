#pragma once
#include "IVideoStream.h"
#include "IMultiVideoSource.h"

namespace TopGear
{
	class VideoSource final 
		: public IVideoStream
	{
	public:
		VideoSource(std::shared_ptr<IMultiVideoSource> &reader, uint32_t index);
		virtual ~VideoSource() {}

		virtual bool StartStream(int formatIndex) override;
		virtual bool StopStream() override;
		virtual bool IsStreaming() const override;
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc) override;
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
		virtual const std::vector<VideoFormat>& GetAllFormats() const override
		{
			return formats;
		}
		virtual const VideoFormat& GetCurrentFormat() const override
		{
			return formats[currentFormatIndex];
		}
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;
	private:
		void OnFrame(IVideoFrameRef &frame);
		std::shared_ptr<IMultiVideoSource> parent;
		uint32_t streamIndex;
		const std::vector<VideoFormat> &formats;
		int currentFormatIndex = 0;
		VideoFrameCallbackFn fncb = nullptr;
	};
}

