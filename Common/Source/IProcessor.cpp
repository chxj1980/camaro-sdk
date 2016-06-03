#include "IProcessor.h"

using namespace TopGear;

ProcessorContainer::ProcessorContainer(IProcessable *owner) :
    host(owner?*owner:*this), done(false)
{}

ProcessorContainer::~ProcessorContainer()
{
    done = true;
    std::unique_lock<std::mutex> lck(mtx);
    cv.notify_all();
    for(auto &p : processors)
        if (p.second.joinable())
            p.second.join();
}

void ProcessorContainer::Register(std::shared_ptr<IProcessor> &processor)
{
    for(auto &p : processors)
        if (p.first == processor)
            return;
    parameters.emplace_back(std::vector<IVideoFramePtr>());
    processors.emplace_back(std::make_pair(processor,
        std::thread(&ProcessorContainer::ProcessWorker, this, count, std::ref(parameters[count]))));
    ++count;
}

void ProcessorContainer::Notify(std::vector<IVideoFramePtr> &payload)
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

void ProcessorContainer::ProcessWorker(int index, std::vector<IVideoFramePtr> &payload)
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

ProcessorFork::ProcessorFork(std::shared_ptr<IProcessor> &p) :
    processor(p), running(false), processing(false)
{}

void ProcessorFork::AddDescendant(std::shared_ptr<IProcessor> &p)
{
    processorList.push_back(p);
    pool.AddThread();
}

bool ProcessorFork::Process(IProcessable &sender, std::vector<IVideoFramePtr> source)
{
    if (!running)
        return false;
    processing = true;
    auto suc = processor->Process(sender, source);
    if (!suc)
    {
        processing = false;
        return false;
    }
    auto ipr =  std::dynamic_pointer_cast<IProcessorResult>(processor);
    if (ipr)
        ipr->GetResult(source);
    for(size_t i = 0 ; i<processorList.size(); ++i)
    {
        auto f = pool.Submit(std::bind(&IProcessor::Process, processorList[i].get(), std::ref(sender), source), i);
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
    return next->Process(sender, source);
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
