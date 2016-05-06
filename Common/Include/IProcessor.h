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
        explicit ProcessorContainer(IProcessable *owner = nullptr);
        virtual ~ProcessorContainer();

        virtual void Register(std::shared_ptr<IProcessor> &processor) override;
        virtual void Notify(std::vector<IVideoFramePtr> &payload) override;
    private:
        IProcessable &host;
        std::vector<std::pair<std::shared_ptr<IProcessor>, std::thread>> processors;
        std::vector<std::vector<IVideoFramePtr>> parameters;
        std::atomic_bool done;
        std::mutex mtx;
        std::condition_variable cv;
        int count = 0;

        void ProcessWorker(int index, std::vector<IVideoFramePtr> &payload);
    };

    class ProcessorFork final
            : public IProcessor
    {
    public:
        explicit ProcessorFork(std::shared_ptr<IProcessor> &p);
        virtual ~ProcessorFork() = default;

        void AddDescendant(std::shared_ptr<IProcessor> &p);
        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source) override;
    private:
        std::shared_ptr<IProcessor> processor;
        std::vector<std::shared_ptr<IProcessor>> processorList;
        ThreadPool pool;
    };

    class ProcessorNode final
            : public IProcessor
    {
    public:
        explicit ProcessorNode(std::shared_ptr<IProcessor> &p);
        virtual ~ProcessorNode() = default;

        void AddDescendant(std::shared_ptr<IProcessor> &p);
        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source);

    private:
        std::shared_ptr<IProcessor> processor;
        std::shared_ptr<IProcessor> next;
    };
}
