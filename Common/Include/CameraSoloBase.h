#pragma once
#include "CameraBase.h"

namespace TopGear
{
	class CameraSoloBase: public CameraBase
	{
	protected:
		explicit CameraSoloBase(std::shared_ptr<IVideoStream> &vs, Configuration &con = Configuration::NullObject())
			:CameraBase(con), pReader(vs)
		{
			videoStreams.emplace_back(vs);
		}
		std::shared_ptr<IVideoStream> pReader;
	public:
		virtual bool StartStream() override
		{
			return pReader->StartStream();
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
