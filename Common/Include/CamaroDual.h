#pragma once
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraBase.h"
#include "BufferQueue.h"
#include "IProcessor.h"

namespace TopGear
{
	class CamaroDual :
		public CameraBase,
		public IProcessable,
		public TopGear::ICameraControl,
		public IDeviceControl

	{
	public:
		virtual void Notify(std::vector<IVideoFramePtr>& payload) override;
		virtual void Register(std::shared_ptr<IProcessor>& p) override;
	protected:
		static const uint16_t RESYNC_NUM = 900;
		std::shared_ptr<TopGear::ICameraControl> masterCC;
		std::shared_ptr<TopGear::ICameraControl> slaveCC;
		std::shared_ptr<IDeviceControl> masterDC;
		std::shared_ptr<IDeviceControl> slaveDC;
		std::shared_ptr<IProcessor> processor;

		void FrameWatcher();
		std::thread frameWatchThread;
		bool threadOn = false;
		BufferQueue<std::pair<int, IVideoFramePtr>> frameBuffer;
		void OnMasterFrame(IVideoStream &master, std::vector<IVideoFramePtr> &frames);
		void OnSlaveFrame(IVideoStream &slave, std::vector<IVideoFramePtr> &frames);
		VideoFrameCallbackFn fnCb = nullptr;
	public:
		CamaroDual(std::shared_ptr<IVideoStream> &master, std::shared_ptr<IVideoStream> &slave);
		virtual ~CamaroDual();
		virtual int Flip(bool vertical, bool horizontal) override;
		virtual int GetExposure(uint16_t& val) override;
		virtual int SetExposure(uint16_t val) override;
		virtual int GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB) override;
		virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) override;
		virtual int SetSensorTrigger(uint8_t level) override;
		virtual int SetResyncNumber(uint16_t resyncNum) override;
		virtual int QueryDeviceRole() override;
		virtual std::string QueryDeviceInfo() override;
		virtual bool StartStream() override;
		virtual bool StopStream() override;
		virtual bool IsStreaming() const override;
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc = "") override;
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
		virtual const std::vector<VideoFormat>& GetAllFormats() const override;
		virtual const VideoFormat &GetCurrentFormat() const override;
		virtual bool SetCurrentFormat(uint32_t formatIndex) override;
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;
	};
}

