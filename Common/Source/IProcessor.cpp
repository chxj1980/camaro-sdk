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
    processor(p)
{}

void ProcessorFork::AddDescendant(std::shared_ptr<IProcessor> &p)
{
    processorList.push_back(p);
    pool.AddThread();
}

bool ProcessorFork::Process(IProcessable &sender, std::vector<IVideoFramePtr> source)
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

ProcessorNode::ProcessorNode(std::shared_ptr<IProcessor> &p) :
    processor(p)
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
