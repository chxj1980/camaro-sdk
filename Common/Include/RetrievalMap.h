#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <utility>
#include <functional>

#include "IVideoFrame.h"

namespace TopGear
{
    enum class RetrievalMapType
    {
        Timestamp,
        FrameIndex,
    };

    class RetrievalMap final
    {
    public:
        typedef std::map<uint64_t, std::pair<int, std::weak_ptr<IVideoFrame>>> SMAP;

        RetrievalMap() {}
        RetrievalMap & operator =(const RetrievalMap &&rhs);
        ~RetrievalMap() {}

        void Clear();
        void Register(int slot ,std::shared_ptr<IVideoFrame> &frame, uint64_t tm, uint64_t fi);
        void Unregister(int slot);

        bool RetrieveNext(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame);
        bool RetrievePrevious(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame);
    private:
        SMAP timeMap;
        SMAP frameIndexMap;
        std::mutex mtx;

        bool RetrieveOne(RetrievalMapType type, uint64_t key, std::shared_ptr<IVideoFrame> &frame,
                         std::function<bool(SMAP::iterator &, SMAP *)> operation);
    };

}

