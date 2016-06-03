#pragma once
#include <vector>
#include "IVideoFrame.h"
#include "ThreadPool.h"

namespace TopGear
{
    template<typename T>
    class IProcessable;

    template<typename T>
	class IProcessor
	{
	public:
		virtual ~IProcessor() = default;
        //virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> source) = 0;
        //virtual bool Process(T &source) = 0;
        virtual bool Process(std::shared_ptr<T> &source) = 0;
        virtual bool IsRunning() const = 0;
        virtual bool Run() = 0;
        virtual bool Stop() = 0;
	};

    template<typename T>
    class IProcessorResult
    {
    public:
        virtual ~IProcessorResult() = default;
        //virtual bool GetResult(T &result) = 0;
        virtual std::shared_ptr<T> GetResult() = 0;
        virtual void PostProcess(bool missingAny) = 0;
    };

    template<typename T>
	class IProcessable
	{
	public:
		virtual ~IProcessable() = default;
        virtual void Register(std::shared_ptr<IProcessor<T>> &processor) = 0;
        virtual void Notify(T &payload) = 0;
    };


    template<typename T>
    class ProcessorContainer
            : public IProcessable<T>
    {
    public:
        explicit ProcessorContainer()
            :done(false)
        {}
        virtual ~ProcessorContainer();

        virtual void Register(std::shared_ptr<IProcessor<T>> &processor) override;
        virtual void Notify(T &payload) override;
    private:
        IProcessable &host;
        std::vector<std::pair<std::shared_ptr<IProcessor<T>>, std::thread>> processors;
        std::shared_ptr<T> parameter;
        std::atomic_bool done;
        std::mutex mtx;
        std::condition_variable cv;
        int count = 0;

        void ProcessWorker(int index);
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

        virtual bool Process(std::shared_ptr<T> source) override;
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

        virtual bool Process(std::shared_ptr<T> source);
        virtual bool IsRunning() const override { return running; }
        virtual bool Run() override;
        virtual bool Stop() override;
    private:
        std::shared_ptr<IProcessor<P>> processor;
        std::shared_ptr<IProcessor<D>> next;
        std::atomic_bool running;
        std::atomic_bool processing;
    };

    template<typename T>
    ProcessorContainer<T>::~ProcessorContainer()
    {
        done = true;
        std::unique_lock<std::mutex> lck(mtx);
        cv.notify_all();
        for(auto &p : processors)
            if (p.second.joinable())
                p.second.join();
    }

    template<typename T>
    void ProcessorContainer<T>::Register(std::shared_ptr<IProcessor<T>> &processor)
    {
        for(auto &p : processors)
            if (p.first == processor)
                return;
        processors.emplace_back(std::make_pair(processor,
            std::thread(&ProcessorContainer<T>::ProcessWorker, this, count)));
        ++count;
    }

    template<typename T>
    void ProcessorContainer<T>::Notify(T &payload)
    {
        if (processors.empty())
            return;
        std::unique_lock<std::mutex> lck(mtx);
        parameter = payload;
        cv.notify_all();
    }

    template<typename T>
    void ProcessorContainer<T>::ProcessWorker(int index)
    {
        while (!done)
        {
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck); //, [&]() { return !payload.empty();});
            auto source = parameter;
            lck.unlock();
            processors[index].first->Process(source);
        }
    }

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
    bool ProcessorFork<P, D>::Process(P &source)
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
            auto f = pool.Submit(std::bind(&IProcessor<D>::Process, processorList[i].get(), std::ref(sender), source), i);
            f.valid()
        }
        processing = false;
        return true;
    }

    bool ProcessorFork::Run()
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

    bool ProcessorFork::Stop()
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

    ProcessorNode::ProcessorNode(std::shared_ptr<IProcessor> &p) :
        processor(p), running(false), processing(false)
    {}

    void ProcessorNode::AddDescendant(std::shared_ptr<IProcessor> &p)
    {
        next = p;
    }

    bool ProcessorNode::Process(IProcessable &sender, std::vector<IVideoFramePtr> source)
    {
        auto suc = processor->Process(sender, source);
        if (!suc)
            return false;
        if (next == nullptr)
            return true;
        auto ipr =  std::dynamic_pointer_cast<IProcessorResult>(processor);
        if (ipr)
            ipr->GetResult(source);
        return next->Process(source);
    }

    bool ProcessorNode::Run()
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

    bool ProcessorNode::Stop()
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
