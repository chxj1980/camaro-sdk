#pragma once

#include "CameraBase.h"
#include "IDeviceControl.h"
#include "ExtensionAccessAdapter.h"
#include <thread>
#include "BufferQueue.h"
#include <atomic>

namespace TopGear
{
	class ImpalaE
		: public CameraBase,
		 public IDeviceControl
	{
	public:
		virtual bool SetControl(std::string name, IPropertyData& val) override;
		virtual bool SetControl(std::string name, IPropertyData&& val) override;
		virtual bool GetControl(std::string name, IPropertyData& val) override;
		virtual bool StartStream() override;
		virtual bool StopStream() override;
		virtual bool IsStreaming() const override;
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc) override;
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
		virtual const std::vector<VideoFormat>& GetAllFormats() const override;
		virtual const VideoFormat& GetCurrentFormat() const override;
		virtual bool SetCurrentFormat(uint32_t formatIndex) override;
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

		explicit ImpalaE(std::vector<std::shared_ptr<IVideoStream>>& vs,
						 std::shared_ptr<IExtensionAccess> &ex);
		virtual ~ImpalaE() {}
	protected:
		void OnFrame(IVideoStream &parent, std::vector<IVideoFramePtr> &frames);
		std::shared_ptr<IExtensionAccess> xuAccess;
		VideoFrameCallbackFn fnCb = nullptr;
		std::vector<VideoFormat> formats;
        std::vector<std::pair<int,int>> selectedFormats;
	private:
		void FrameWatcher();
		std::thread frameWatchThread;
        std::atomic<bool> streaming;
		BufferQueue<std::pair<int, IVideoFramePtr>> frameBuffer;
	};
}
