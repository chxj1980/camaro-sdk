#pragma once

#include "IProcessor.h"

namespace TopGear
{

template<typename T>
class ProcessorBase
        : public IProcessor<T>
{
public:
    ProcessorBase() :
        running(false), processing(false)
    {}
    virtual ~ProcessorBase() = default;
    virtual bool ProcessImp(T &source) = 0;
    virtual bool Process(std::shared_ptr<T> source) final
    {
        if (!running)
            return false;
        processing = true;
        auto suc = ProcessImp(*source);
        processing = false;
        return suc;
    }
    virtual bool IsRunning() const override { return running; }
    virtual bool Run() override
    {
        if (running)
            return true;
        running = true;
        return true;
    }
    virtual bool Stop() override
    {
        if (!running)
            return true;
        running = false;
        while (processing)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    }
private:
    std::atomic_bool running;
    std::atomic_bool processing;
};
}


