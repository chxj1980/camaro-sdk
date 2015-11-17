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
	public:
		virtual ~CameraBase()
		{
		}

	};
}
