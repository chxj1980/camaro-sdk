#pragma once
#include "CameraBase.h"
#include "WatchDog.h"
#include "MobileChecker.h"

namespace TopGear
{
    class CameraSoloBase:
            public CameraBase,
            public IWatch,
            public IMobile
	{
	protected:
		explicit CameraSoloBase(std::shared_ptr<IVideoStream> &vs, CameraProfile &con = CameraProfile::NullObject())
            :CameraBase(con), pReader(vs)
		{
			videoStreams.emplace_back(vs);
            pReader->RegisterFrameCallback(std::bind(&CameraSoloBase::OnFrame, this, std::placeholders::_1, std::placeholders::_2));
		}

        virtual void PostProcess(std::vector<IVideoFramePtr> &frames)
        {
            (void)frames;
        }

		std::shared_ptr<IVideoStream> pReader;
        VideoFrameCallbackFn fncb = nullptr;
        TimeoutCallbackFn tcb = nullptr;
        std::chrono::seconds interval;
        WatchDog watchdog;
        MobileChecker checker;
    private:
        void OnFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames)
        {
            (void)source;
            checker.EndLatency();
            if (tcb)
                watchdog.Feed();
            PostProcess(frames);
            auto vp = std::make_shared<std::vector<IVideoFramePtr>>(std::move(frames));
            Notify(vp);
            if (fncb)
                fncb(*this, *vp);
        }

	public:
		virtual bool StartStream() override
		{
            if (tcb)
                watchdog.Start(interval, std::bind(tcb, std::ref(*this)));
            auto res = pReader->StartStream();
            checker.BeginLatency();
            return res;
		}
		virtual bool StopStream() override
		{
            if (tcb)
                watchdog.Stop();
			return pReader->StopStream();
		}
		virtual bool IsStreaming() const override
		{
			return pReader->IsStreaming();
		}
        virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override
        {
            fncb = fn;
        }

        virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override
        {
            fncb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
        }

        virtual void RegisterTimeoutCallback(const TimeoutCallbackFn &fn, std::chrono::seconds timeout) override
        {
            tcb = fn;
            interval = std::move(timeout);
        }

        virtual void StartMove() override
        {
            checker.StartMove();
        }
        virtual void StopMove() override
        {
            checker.StopMove();
        }
        virtual bool IsSteady() override
        {
            return checker.IsSteady();
        }

        virtual ~CameraSoloBase() = default;
	};
}
