#pragma once
#include "IVideoStream.h"
#include "Configuration.h"

namespace TopGear
{
	class CameraBase : public IVideoStream
	{
	protected:
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &vss,
							Configuration &con = Configuration::NullObject())
			: videoStreams(vss), config(con)
		{
		}
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &&vss,
							Configuration &con = Configuration::NullObject())
			: videoStreams(vss), config(con)
		{
		}
		explicit CameraBase(Configuration &con = Configuration::NullObject()) 
			: config(con)
		{}
		std::vector<std::shared_ptr<IVideoStream>> videoStreams;
		Configuration &config;
	public:
		virtual ~CameraBase()
		{
		}
	};
}
