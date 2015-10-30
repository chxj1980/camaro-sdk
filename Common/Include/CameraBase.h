#pragma once
#include "IVideoStream.h"

namespace TopGear
{
	class CameraBase : public IVideoStream
	{
	protected:
		explicit CameraBase(std::shared_ptr<IVideoStream> &vs)
			:pReader(vs) {}
		std::shared_ptr<IVideoStream> pReader;
	public:
		virtual bool StartStream(int formatIndex) override
		{
			return pReader->StartStream(formatIndex);
		}
		virtual bool StopStream() override
		{
			return pReader->StopStream();
		}
		virtual bool IsStreaming() const override
		{
			return pReader->IsStreaming();
		}
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override
		{
			pReader->RegisterFrameCallback(fn);
		}
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override
		{
			pReader->RegisterFrameCallback(pCB);
		}

		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc = "") override
		{
			return pReader->GetOptimizedFormatIndex(format, fourcc);
		}
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override
		{
			return pReader->GetMatchedFormatIndex(format);
		}
		virtual const std::vector<VideoFormat>& GetAllFormats() const override
		{
			return pReader->GetAllFormats();
		}
		virtual ~CameraBase()
		{
			pReader->StopStream();
		}

	};
}
