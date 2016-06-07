#pragma once
#include <vector>
#include "IVideoFrame.h"
#include "ThreadPool.h"

namespace TopGear
{
    template<typename T>
	class IProcessor
	{
	public:
		virtual ~IProcessor() = default;
        virtual bool Process(std::shared_ptr<T> source) = 0;
        virtual bool IsRunning() const = 0;
        virtual bool Run() = 0;
        virtual bool Stop() = 0;
	};

    template<typename T>
    class IProcessorResult
    {
    public:
        virtual ~IProcessorResult() = default;
        virtual std::shared_ptr<T> GetResult() = 0;
        //virtual void PostProcess(bool missingAny) = 0;
    };

    template<typename T, typename P, typename D>
    class IPipelineNode
    {
    public:
        virtual void AddDescendant(std::shared_ptr<IProcessor<D>> &p) = 0;
        virtual std::shared_ptr<IProcessor<P>> &GetProcessor() = 0;
        virtual T& GetDescendant() = 0;
    };

    template<typename P, typename D>
    class ProcessorFork
            : public IProcessor<P>,
              public IPipelineNode<std::vector<std::shared_ptr<IProcessor<D>>>, P, D>
    {
    public:
        explicit ProcessorFork(std::shared_ptr<IProcessor<P>> &p);
        virtual ~ProcessorFork() = default;

        virtual void AddDescendant(std::shared_ptr<IProcessor<D>> &p) override;
        virtual std::shared_ptr<IProcessor<P>> &GetProcessor() override { return processor; }
        virtual std::vector<std::shared_ptr<IProcessor<D>>> &GetDescendant() override
        {
            return processorList;
        }

        virtual bool Process(std::shared_ptr<P> source) override;
        virtual bool IsRunning() const override { return running; }
        virtual bool Run() override;
        virtual bool Stop() override;
    private:
        std::shared_ptr<IProcessor<P>> processor;
        std::vector<std::shared_ptr<IProcessor<D>>> processorList;
        ThreadPool pool;
        std::atomic_bool running;
        std::atomic_bool processing;
    };

    template<typename P, typename D>
    class ProcessorNode
            : public IProcessor<P>,
              public IPipelineNode<std::shared_ptr<IProcessor<D>>, P, D>
    {
    public:
        explicit ProcessorNode(std::shared_ptr<IProcessor<P>> &p);
        virtual ~ProcessorNode() = default;

        virtual void AddDescendant(std::shared_ptr<IProcessor<D>> &p) override;
        virtual std::shared_ptr<IProcessor<P>> &GetProcessor() override { return processor; }
        virtual std::shared_ptr<IProcessor<D>> &GetDescendant() override { return next; }

        virtual bool Process(std::shared_ptr<P> source);
        virtual bool IsRunning() const override { return running; }
        virtual bool Run() override;
        virtual bool Stop() override;
    private:
        std::shared_ptr<IProcessor<P>> processor;
        std::shared_ptr<IProcessor<D>> next;
        std::atomic_bool running;
        std::atomic_bool processing;
    };

    template<typename P, typename D>
    ProcessorFork<P, D>::ProcessorFork(std::shared_ptr<IProcessor<P>> &p) :
        processor(p), running(false), processing(false)
    {}

    template<typename P, typename D>
    void ProcessorFork<P, D>::AddDescendant(std::shared_ptr<IProcessor<D>> &p)
    {
        processorList.push_back(p);
        pool.AddThread();
    }

    template<typename P, typename D>
    bool ProcessorFork<P, D>::Process(std::shared_ptr<P> source)
    {
        if (!running)
            return false;
        processing = true;
        auto suc = processor->Process(source);
        if (!suc)
        {
            processing = false;
            return false;
        }
        auto ipr =  std::dynamic_pointer_cast<IProcessorResult<D>>(processor);
        if (ipr)
            ipr->GetResult(source);
        for(size_t i = 0 ; i<processorList.size(); ++i)
        {
            //auto future =
            pool.Submit(
                std::bind(&IProcessor<D>::Process, processorList[i].get(), source), i);
            //future.valid();
        }
        processing = false;
        return true;
    }

    template<typename P, typename D>
    bool ProcessorFork<P, D>::Run()
    {
        if (running)
            return true;
        for(size_t i = 0 ; i<processorList.size(); ++i)
            if (!processorList[i]->Run())
                return false;
        if (!processor->Run())
            return false;
        running = true;
        return true;
    }

    template<typename P, typename D>
    bool ProcessorFork<P, D>::Stop()
    {
        if (!running)
            return true;
        running = false;
        while (processing)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (!processor->Stop())
        {
            running = true;
            return false;
        }
        bool suc = true;
        for(size_t i = 0 ; i<processorList.size(); ++i)
            if (!processorList[i]->Stop())
                suc = false;
        return suc;
    }

    template<typename P, typename D>
    ProcessorNode<P, D>::ProcessorNode(std::shared_ptr<IProcessor<P>> &p) :
        processor(p), running(false), processing(false)
    {}

    template<typename P, typename D>
    void ProcessorNode<P, D>::AddDescendant(std::shared_ptr<IProcessor<D>> &p)
    {
        next = p;
    }

    template<typename P, typename D>
    bool ProcessorNode<P, D>::Process(std::shared_ptr<P> source)
    {
        auto suc = processor->Process(source);
        if (!suc)
            return false;
        if (next == nullptr)
            return true;
        auto ipr =  std::dynamic_pointer_cast<IProcessorResult<D>>(processor);
        if (ipr)
            ipr->GetResult(source);
        return next->Process(source);
    }

    template<typename P, typename D>
    bool ProcessorNode<P, D>::Run()
    {
        auto suc = next->Run();
        if (!suc)
            return false;
        suc = processor->Run();
        if (!suc)
            return false;
        running = true;
        return true;
    }

    template<typename P, typename D>
    bool ProcessorNode<P, D>::Stop()
    {
        running = false;
        while (processing)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto suc = processor->Stop();
        if (!suc)
        {
            running = true;
            return false;
        }
        return next->Stop();
    }

}
