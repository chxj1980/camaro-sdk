#pragma once
#include <vector>
#include "IVideoFrame.h"
#include "ThreadPool.h"

namespace TopGear
{
    class IProcessable;

	class IProcessor
	{
	public:
		virtual ~IProcessor() = default;
        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source) = 0;
	};

    class IProcessorResult
    {
    public:
        virtual ~IProcessorResult() = default;
        virtual bool GetResult(std::vector<IVideoFramePtr> &result) = 0;
    };

	class IProcessable
	{
	public:
		virtual ~IProcessable() = default;
		virtual void Register(std::shared_ptr<IProcessor> &processor) = 0;
		virtual void Notify(std::vector<IVideoFramePtr> &payload) = 0;
    };

    class ProcessorContainer
            : public IProcessable
    {
    public:
        ProcessorContainer(IProcessable *owner = nullptr) :
            host(owner?*owner:*this), done(false)
        {}
        virtual ~ProcessorContainer()
        {
            done = true;
            std::unique_lock<std::mutex> lck(mtx);
            cv.notify_all();
            for(auto &p : processors)
                if (p.second.joinable())
                    p.second.join();
        }

        virtual void Register(std::shared_ptr<IProcessor> &processor) override
        {
            for(auto &p : processors)
                if (p.first == processor)
                    return;
            parameters.emplace_back(std::vector<IVideoFramePtr>());
            processors.emplace_back(std::make_pair(processor,
                std::thread(&ProcessorContainer::ProcessWorker, this, count, std::ref(parameters[count]))));
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
        }
    private:
        IProcessable &host;
        std::vector<std::pair<std::shared_ptr<IProcessor>, std::thread>> processors;
        std::vector<std::vector<IVideoFramePtr>> parameters;
        std::atomic_bool done;
        std::mutex mtx;
        std::condition_variable cv;
        int count = 0;

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
                processors[index].first->Process(host, frames);
            }
        }
    };

    class ProcessorNode final
            : public IProcessor
    {
    public:
        explicit ProcessorNode(std::shared_ptr<IProcessor> &p) :
            processor(p)
        {}

        virtual ~ProcessorNode() = default;

        void AddDescendant(std::shared_ptr<IProcessor> &p)
        {
            processorList.push_back(p);
            pool.AddThread();
        }

        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source) override
        {
            auto suc = processor->Process(sender, source);
            if (!suc)
                return false;
            auto ipr =  std::dynamic_pointer_cast<IProcessorResult>(processor);
            if (ipr)
                ipr->GetResult(source);
            for(size_t i = 0 ; i<processorList.size(); ++i)
                pool.Submit(std::bind(&IProcessor::Process, processorList[i].get(), std::ref(sender), source), i);
            return true;
        }

    private:
        std::shared_ptr<IProcessor> processor;
        std::vector<std::shared_ptr<IProcessor>> processorList;
        ThreadPool pool;
    };
}
