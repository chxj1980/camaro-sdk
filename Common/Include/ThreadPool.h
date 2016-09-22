#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <functional>
#include <typeinfo>
#include <utility>
#include "BufferQueue.h"

namespace TopGear
{
    class JoinThreads
    {
        std::vector<std::thread>& threads;
    public:
        explicit JoinThreads(std::vector<std::thread>& v) :
            threads(v)
        {}
        ~JoinThreads()
        {
            for(auto &t : threads)
                if (t.joinable())
                    t.join();
        }
    };

    class FunctionWrapper
    {
    private:
        struct ImpBase
        {
            virtual void call() = 0;
            virtual ~ImpBase() {}
        };

        std::shared_ptr<ImpBase> imp;

        template <typename F>
        struct ImpType : ImpBase
        {
            F f;
            ImpType(F&& f_) : f(std::move(f_)) {}
            void call() override { f(); }
        };
    public:
        template <typename F>
        FunctionWrapper(F&& f) :
            imp(new ImpType<F>(std::move(f)))
        {}

        void operator ()() { imp->call(); }

        FunctionWrapper() = default;

        FunctionWrapper(FunctionWrapper&& other) :
            imp(std::move(other.imp))
        {}

        FunctionWrapper& operator =(FunctionWrapper&& other)
        {
            imp = std::move(other.imp);
            return *this;
        }

        FunctionWrapper(const FunctionWrapper& other):
            imp(other.imp)
        {}
        FunctionWrapper(FunctionWrapper&) = delete;
        FunctionWrapper& operator =(const FunctionWrapper&) = delete;
    };

    class ThreadPool
    {
    private:
        BufferQueue<std::pair<FunctionWrapper,size_t>> queue;
        std::atomic_bool done;
        std::vector<std::thread> threads;
        std::vector<size_t> tokens;
        std::mutex mtx;
        JoinThreads joiner;

        void Worker()
        {
            while(!done)
            {
                std::pair<FunctionWrapper,size_t> task;
                if (queue.Pop(task))
                {
                    task.first();
                    //Remove task token from list
                    std::lock_guard<std::mutex> lg(mtx);
                    for(auto it = tokens.begin(); it!=tokens.end();++it)
                        if (*it == task.second)
                        {
                            tokens.erase(it);
                            break;
                        }
                }
                else
                    break;
            }
        }

    public:
        explicit ThreadPool(int count = 0) :
            done(false), joiner(threads)
        {
            try
            {
                for(auto i=0;i<count;++i)
                    AddThread();
            }
            catch(...)
            {
                done = true;
                throw;
            }
        }
        ~ThreadPool()
        {
            done = true;
            for(auto &t:threads)
                if (t.joinable())
                    queue.Discard();
        }

        void AddThread()
        {
            threads.emplace_back(std::thread(&ThreadPool::Worker, this));
        }

        template <typename FunctionType>
        std::future<typename std::result_of<FunctionType()>::type> Submit(FunctionType f, size_t token)
        {
            typedef typename std::result_of<FunctionType()>::type result_type;

            {
                std::lock_guard<std::mutex> lg(mtx);
                for(auto &t : tokens)
                    if (t==token)
                        return {};
                //Add task token to list
                tokens.push_back(token);
            }

            std::packaged_task<result_type()> task(std::move(f));
            std::future<result_type> res(task.get_future());
            queue.Push(std::make_pair(std::move(task), token));
            return res;
        }
    };


}

