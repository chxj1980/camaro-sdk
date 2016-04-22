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
        std::vector<std::pair<std::shared_ptr<IProcessor>, std::thread>> processors;
        std::vector<std::vector<IVideoFramePtr>> parameters;
	public:
        virtual void Register(std::shared_ptr<IProcessor> &processor) override
        {
            static int count = 0;
            for(auto &p : processors)
                if (p.first == processor)
                    return;
            parameters.emplace_back(std::vector<IVideoFramePtr>());
            processors.emplace_back(std::make_pair(processor,
                std::thread(&CameraBase::ProcessWorker, this, count, std::ref(parameters[count]))));
            ++count;
        }
        virtual void Notify(std::vector<IVideoFramePtr> &payload) override
        {
            if (processors.empty())
                return;
            std::unique_lock<std::mutex> lck(mtx);
            for(auto &p : parameters)
            {
                p = payload;
            }
            cv.notify_all();
            //lck.release();
        }
		virtual ~CameraBase()
		{
            done = true;
            std::unique_lock<std::mutex> lck(mtx);
            cv.notify_all();
            for(auto &p : processors)
                if (p.second.joinable())
                    p.second.join();
		}
      private:
        std::atomic_bool done;
        std::mutex mtx;
        std::condition_variable cv;
        void ProcessWorker(int index, std::vector<IVideoFramePtr> &payload)
        {
            while (!done)
            {
                std::unique_lock<std::mutex> lck(mtx);
                cv.wait(lck); //, [&]() { return !payload.empty();});
                if (payload.empty())
                    continue;
                std::vector<IVideoFramePtr> frames = std::move(payload);
                payload = std::vector<IVideoFramePtr>();
                lck.release();
                processors[index].first->Process(*this, frames);
            }
        }
	};
}
