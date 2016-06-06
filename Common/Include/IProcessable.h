#pragma once

#include "IProcessor.h"


namespace TopGear
{
    template<typename T>
    class IProcessable
    {
    public:
        virtual ~IProcessable() = default;
        virtual void Register(std::shared_ptr<IProcessor<T>> &processor) = 0;
        virtual void Notify(std::shared_ptr<T> &payload) = 0;
    };

    template<typename T>
    class ProcessorContainer
            : public IProcessable<T>
    {
    public:
        ProcessorContainer()
            :done(false)
        {}
        virtual ~ProcessorContainer();

        virtual void Register(std::shared_ptr<IProcessor<T>> &processor) override;
        virtual void Notify(std::shared_ptr<T> &payload) override;
    private:
        std::vector<std::pair<std::shared_ptr<IProcessor<T>>, std::thread>> processors;
        std::shared_ptr<T> parameter;
        std::atomic_bool done;
        std::mutex mtx;
        std::condition_variable cv;
        int count = 0;

        void ProcessWorker(int index);
    };

    template<typename T>
    ProcessorContainer<T>::~ProcessorContainer()
    {
        std::unique_lock<std::mutex> lck(mtx);
        done = true;
        cv.notify_all();
        lck.unlock();
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
    void ProcessorContainer<T>::Notify(std::shared_ptr<T> &payload)
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
        while (true)
        {
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck); //, [&]() { return !payload.empty();});
            if (done)
                return;
            auto source = parameter;
            lck.unlock();
            processors[index].first->Process(source);
        }
    }
}

