#pragma once
#include "CameraBase.h"

namespace TopGear
{
	class CameraSoloBase: public CameraBase
	{
	protected:
		explicit CameraSoloBase(std::shared_ptr<IVideoStream> &vs)
			:CameraBase(), pReader(vs)
		{
			videoStreams.emplace_back(vs);
		}
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

		virtual ~CameraSoloBase()
		{
			StopStream();
		}
	};
}
