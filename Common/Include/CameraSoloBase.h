#pragma once
#include "CameraBase.h"

namespace TopGear
{
	class CameraSoloBase: public CameraBase
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
        VideoFrameCallbackFn fncb;
    private:
        void OnFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames)
        {
            (void)source;
            PostProcess(frames);
            Notify(frames);
            if (fncb)
                fncb(*this, frames);
        }
	public:
		virtual bool StartStream() override
		{
			return pReader->StartStream();
		}
		virtual bool StopStream() override
		{
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

		virtual ~CameraSoloBase()
		{
		}
	};
}
