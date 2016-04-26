#include "RetrievalMap.h"

using namespace TopGear;

RetrievalMap & RetrievalMap::operator =(const RetrievalMap &&rhs)
{
    timeMap = rhs.timeMap;
    frameIndexMap = rhs.frameIndexMap;
    return *this;
}

void RetrievalMap::Clear()
{
    timeMap.clear();
    frameIndexMap.clear();
}

void RetrievalMap::Register(int slot ,std::shared_ptr<IVideoFrame> &frame, uint64_t tm, uint64_t fi)
{
    auto item = std::make_pair(slot, std::weak_ptr<IVideoFrame>(frame));
    std::lock_guard<std::mutex> lg(mtx);
    timeMap[tm] = item;
    frameIndexMap[fi] = item;
}

void RetrievalMap::Unregister(int slot)
{
    std::lock_guard<std::mutex> lg(mtx);
    for(auto it = timeMap.begin(); it!=timeMap.end(); ++it)
        if (it->second.first == slot)
            timeMap.erase(it);
    for(auto it = frameIndexMap.begin(); it!=frameIndexMap.end(); ++it)
        if (it->second.first == slot)
            frameIndexMap.erase(it);
}

bool RetrievalMap::RetrieveNext(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame)
{
    return RetrieveOne(type, key, frame,
                       [](SMAP::iterator &it, SMAP *ptr){ return (++it == ptr->end());});
}

bool RetrievalMap::RetrievePrevious(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame)
{
    return RetrieveOne(type, key, frame,
                       [](SMAP::iterator &it, SMAP *ptr){ return (--it == ptr->end());});
}

bool RetrievalMap::RetrieveOne(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame,
                 std::function<bool(SMAP::iterator &, SMAP *)> operation)
{
    SMAP *pmap = nullptr;
    switch (type)
    {
    case RetrievalMapType::Timestamp:
        pmap = &timeMap;
        break;
    case RetrievalMapType::FrameIndex:
        pmap = &frameIndexMap;
        break;
    default:
        break;
    }
    if (pmap == nullptr)
        return false;
    std::lock_guard<std::mutex> lg(mtx);
    auto found = pmap->find(key);
    if (found == pmap->end())
        return false;

    //Execute operation
    if (operation(found,pmap))
        return false;

    std::weak_ptr<IVideoFrame> wptr = found->second.second;
    if (wptr.expired())
        return false;
    frame = wptr.lock();
    return true;
}
