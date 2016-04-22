#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>

#include "BufferQueue.h"

namespace TopGear
{
    class JoinThreads
    {
    public:
        explicit JoinThreads(std::vector<std::thread> &v)
            : threads(v)
        {}
        ~JoinThreads()
        {
            for(auto &t : threads)
                if (t.joinable())
                    t.join();
        }

    private:
        std::vector<std::thread> &threads;
    };

    template<class Ret>
    class ThreadPool
    {
    public:
        ThreadPool(int num=0)
            : jointer(threads)
        {
            for(auto i=0;i<num;++i)
                threads.emplace_back(std::thread(Worker));
        }

        ~ThreadPool() {}

        int AddThread()
        {
            threads.emplace_back(std::thread(Worker));
            return threads.size()-1;
        }

    private:
        std::atomic_bool done;
        BufferQueue<std::function<Ret()>> workQueue;
        std::vector<std::thread> threads;
        JoinThreads jointer;

        void Worker()
        {
            while (!done)
            {
                std::function<Ret(void)> task;
                if (workQueue.Pop_NoWait(task))
                {
                    task()
                }
                else
                    std::this_thread::yield();
            }
        }
    };
}
