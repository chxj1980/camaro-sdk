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
        virtual bool IsRunning() const = 0;
        virtual bool Run() = 0;
        virtual bool Stop() = 0;
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

    template<class T>
    class IPipelineNode
    {
    public:
        virtual void AddDescendant(std::shared_ptr<IProcessor> &p) = 0;
        virtual std::shared_ptr<IProcessor> &GetProcessor() = 0;
        virtual T& GetDescendant() = 0;
    };

    class ProcessorFork
            : public IProcessor,
              public IPipelineNode<std::vector<std::shared_ptr<IProcessor>>>
    {
    public:
        explicit ProcessorFork(std::shared_ptr<IProcessor> &p);
        virtual ~ProcessorFork() = default;

        virtual void AddDescendant(std::shared_ptr<IProcessor> &p) override;
        virtual std::shared_ptr<IProcessor> &GetProcessor() override;
        virtual std::vector<std::shared_ptr<IProcessor>> &GetDescendant() override;

        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source) override;
        virtual bool IsRunning() const override { return running; }
        virtual bool Run() override;
        virtual bool Stop() override;
    private:
        std::shared_ptr<IProcessor> processor;
        std::vector<std::shared_ptr<IProcessor>> processorList;
        ThreadPool pool;
        std::atomic_bool running;
        std::atomic_bool processing;
    };

    class ProcessorNode
            : public IProcessor,
              public IPipelineNode<std::shared_ptr<IProcessor>>
    {
    public:
        explicit ProcessorNode(std::shared_ptr<IProcessor> &p);
        virtual ~ProcessorNode() = default;

        virtual void AddDescendant(std::shared_ptr<IProcessor> &p) override;
        virtual std::shared_ptr<IProcessor> &GetProcessor() override;
        virtual std::shared_ptr<IProcessor> &GetDescendant() override;

        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source);
        virtual bool IsRunning() const override { return running; }
        virtual bool Run() override;
        virtual bool Stop() override;
    private:
        std::shared_ptr<IProcessor> processor;
        std::shared_ptr<IProcessor> next;
        std::atomic_bool running;
        std::atomic_bool processing;
    };
}
