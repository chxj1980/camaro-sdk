#pragma once
#include "IVideoStream.h"

namespace TopGear
{
	class CameraComboBase: public IVideoStream
	{
	protected:
		explicit CameraComboBase(std::vector<std::shared_ptr<IVideoStream>> &vss)
			: videoStreams(vss)
		{
		}
		explicit CameraComboBase(std::vector<std::shared_ptr<IVideoStream>> &&vss)
			: videoStreams(vss)
		{
		}
		CameraComboBase() {}
		std::vector<std::shared_ptr<IVideoStream>> videoStreams;
		VideoFrameCallbackFn fnCb = nullptr;
		IVideoFrameCallback *pCbobj = nullptr;
	public:
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override
		{
			fnCb = fn;
		}
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override
		{
			pCbobj = pCB;
		}
		virtual ~CameraComboBase()
		{
		}
	};
}
