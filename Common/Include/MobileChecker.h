#pragma once

#include "IMobile.h"

#include <thread>
#include <chrono>
#include <future>
#include <atomic>

namespace TopGear
{
    class MobileChecker : public IMobile
    {
    public:
        MobileChecker() : isMoving(false), isSteady(true)
        {}

        ~MobileChecker()
        {
        }

        virtual void StartMove() override
        {
            if (isMoving.load())
                return;
            isMoving = true;
            move_begin = std::chrono::high_resolution_clock::now();
            delayStatus = std::async(std::launch::async, [&](){
                std::this_thread::sleep_for(latency);
                isSteady = false;
            });
        }

        virtual void StopMove() override
        {
            if (!isMoving.load())
                return;

            move_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now()-move_begin);
            if (delayStatus.wait_for(std::chrono::microseconds::zero())==std::future_status::timeout)
            {
                std::thread([&](){
                    delayStatus.wait();
                    std::this_thread::sleep_for(move_duration);
                    isSteady = true;
                    isMoving = false;
                }).detach();
            }
            else
            {
                std::thread([&](){
                    std::this_thread::sleep_for(move_duration-latency);
                    isSteady = true;
                    isMoving = false;
                }).detach();
            }
        }

        virtual bool IsSteady() override
        {
            return isSteady.load();
        }

        void BeginLatency()
        {
            begin = std::chrono::high_resolution_clock::now();
            firstFrame = true;
        }

        void EndLatency()
        {
            if (firstFrame)
            {
                latency = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::high_resolution_clock::now()-begin);
                firstFrame = false;
            }
        }

    private:
        std::chrono::high_resolution_clock::time_point begin;
        std::chrono::microseconds latency;
        std::chrono::high_resolution_clock::time_point move_begin;
        std::chrono::microseconds move_duration;
        bool firstFrame = false;
        std::future<void> delayStatus;
        std::atomic_bool isMoving;
        std::atomic_bool isSteady;
    };
}

