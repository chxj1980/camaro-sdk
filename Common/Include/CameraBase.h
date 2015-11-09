#pragma once
#include "IVideoStream.h"

namespace TopGear
{
	class CameraBase : public IVideoStream
	{
	protected:
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &vss)
			: videoStreams(vss)
		{
		}
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &&vss)
			: videoStreams(vss)
		{
		}
		CameraBase() {}
		std::vector<std::shared_ptr<IVideoStream>> videoStreams;


		//explicit CameraBase(std::shared_ptr<IVideoStream> &vs)
		//	:pReader(vs) {}
		//std::shared_ptr<IVideoStream> pReader;
	public:

		//virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override
		//{
		//	pReader->RegisterFrameCallback(fn);
		//}
		//virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override
		//{
		//	pReader->RegisterFrameCallback(pCB);
		//}

		virtual ~CameraBase()
		{
		}

	};
}
