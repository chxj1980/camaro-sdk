#pragma once
#include "IVideoStream.h"
#include "CameraBase.h"

namespace TopGear
{
	class StandardUVC :public CameraBase
	{
	public:
		explicit StandardUVC(std::shared_ptr<IVideoStream> &vs)
			: CameraBase(vs) {}
		virtual ~StandardUVC() {}
	};
}

