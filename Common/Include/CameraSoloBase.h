#pragma once
#include "CameraBase.h"
#include <thread>
#include <chrono>

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
			if (!pReader->StartStream(formatIndex))
				return false;
			while (!pReader->IsStreaming())
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			return true;
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
		}
	};
}
