#pragma once
#include "IVideoStream.h"
#include "CameraProfile.h"
#include "IProcessor.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace TopGear
{
    class CameraBase
            : public IVideoStream,
              public IProcessable
	{
	protected:
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &vss,
			CameraProfile &con = CameraProfile::NullObject())
            : videoStreams(vss), config(con), container(this)
		{
		}
		explicit CameraBase(std::vector<std::shared_ptr<IVideoStream>> &&vss,
			CameraProfile &con = CameraProfile::NullObject())
            : videoStreams(vss), config(con), container(this)
		{
		}
		explicit CameraBase(CameraProfile &con = CameraProfile::NullObject())
			: config(con)
		{}
		std::vector<std::shared_ptr<IVideoStream>> videoStreams;
		CameraProfile &config;
        ProcessorContainer container;
	public:
        virtual void Register(std::shared_ptr<IProcessor> &processor) override
        {
            container.Register(processor);
        }
        virtual void Notify(std::vector<IVideoFramePtr> &payload) override
        {
            container.Notify(payload);
        }
		virtual ~CameraBase()
		{
		}
	};
}
