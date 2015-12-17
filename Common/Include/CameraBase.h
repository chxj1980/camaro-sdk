#pragma once
#include "IVideoStream.h"
#include "CameraProfile.h"

namespace TopGear
{
	class CameraBase : public IVideoStream
	{
	protected:
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &vss,
			CameraProfile &con = CameraProfile::NullObject())
			: videoStreams(vss), config(con)
		{
		}
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &&vss,
			CameraProfile &con = CameraProfile::NullObject())
			: videoStreams(vss), config(con)
		{
		}
		explicit CameraBase(CameraProfile &con = CameraProfile::NullObject())
			: config(con)
		{}
		std::vector<std::shared_ptr<IVideoStream>> videoStreams;
		CameraProfile &config;
	public:
		virtual ~CameraBase()
		{
		}
	};
}
