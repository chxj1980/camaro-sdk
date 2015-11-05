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

		virtual ~CameraBase()
		{
			pReader->StopStream();
		}

	};
}
