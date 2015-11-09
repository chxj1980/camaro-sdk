#pragma once

#include "IVideoStream.h"
#include "IExtensionAccess.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraSoloBase.h"
#include "ILowlevelControl.h"


namespace TopGear
{
	class Camaro
		: public CameraSoloBase,
		public TopGear::ICameraControl,
		public IDeviceControl,
		public ILowlevelControl
	{
	public:
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc="") override;
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override;
		virtual const std::vector<VideoFormat>& GetAllFormats() const override;
		virtual const VideoFormat &GetCurrentFormat() const override { return formats[currentFormatIndex]; }

		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

		virtual bool StartStream(int formatIndex) override;
		virtual bool StopStream() override;

		virtual int SetSensorTrigger(uint8_t level) override;
		virtual int SetResyncNumber(uint16_t resyncNum) override;
		virtual int QueryDeviceRole() override;
		virtual std::string QueryDeviceInfo() override;

		//advanced device controls (on EP0)
		virtual int SetRegisters(uint16_t regaddr[], uint16_t regval[], int num) override;
		virtual int GetRegisters(uint16_t regaddr[], uint16_t regval[], int num) override;
		//single register
		virtual int SetRegister(uint16_t regaddr, uint16_t regval) override;
		virtual int GetRegister(uint16_t regaddr, uint16_t &regval) override;
	protected:
		enum class ControlCode
		{
			Trigger = 1,
			DeviceInfo = 2,
			DeviceRole = 3,
			RegisterAccess = 4,
			Resync = 5,
		};
		
		int currentFormatIndex = 0;

		void OnFrame(IVideoStream &parent, std::vector<IVideoFrameRef> &frames);
		std::shared_ptr<IExtensionAccess> extension;
		VideoFrameCallbackFn fnCb = nullptr;
	private:
		static const int EMBEDDED_LINES = 2;
		void ObtainExtendedLines();
		std::vector<VideoFormat> formats;
		int header;
		int footer;
	public:
		virtual int Flip(bool vertical, bool horizontal) override;
		virtual int GetExposure(uint16_t& val) override;
		virtual int SetExposure(uint16_t val) override;
		virtual int GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB) override;
		virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) override;
		Camaro(std::shared_ptr<IVideoStream> &vs, std::shared_ptr<IExtensionAccess> &ex);
		virtual ~Camaro();
	};
}

