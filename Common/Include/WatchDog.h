#pragma once

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace TopGear
{
    class WatchDog
    {
    public:
        WatchDog() {}
        ~WatchDog()
        {
            Stop();
        }

        void Start(std::chrono::seconds interval, std::function<void(void)> &&callback)
        {
            Stop();
            running = true;
            watcher = std::thread([&](std::function<void(void)> fn,
                                      std::chrono::seconds sec)
            {
                std::unique_lock<std::mutex> ul(mtx);
                while (running)
                {
                    if (cv.wait_for(ul, sec)==std::cv_status::timeout)
                    {
                        if (first)
                            continue;
                        running = false;
                        fn();
                        break;
                    }
                }
            }, callback, interval);
        }

        void Stop()
        {
            if (running)
            {
                std::unique_lock<std::mutex> ul(mtx);
                running = false;
                cv.notify_one();
            }
            if (watcher.joinable())
                watcher.join();
        }

        void Feed()
        {
            if (!running)
                return;
            std::unique_lock<std::mutex> ul(mtx);
            first = false;
            cv.notify_one();
        }

    private:
        std::thread watcher;
        std::mutex mtx;
        std::condition_variable cv;
        bool running = false;
        bool first = true;
    };
}

