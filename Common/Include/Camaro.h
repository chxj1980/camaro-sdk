#pragma once

#include "IExtensionAccess.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraSoloBase.h"
#include "ILowlevelControl.h"
#include "ExtensionAccessAdapter.h"


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

		virtual const VideoFormat &GetCurrentFormat() const override;
		virtual bool SetCurrentFormat(uint32_t formatIndex) override;	

		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

		virtual bool StartStream() override;
		virtual bool StopStream() override;

		virtual bool SetControl(std::string name, IPropertyData &val) override;
		virtual bool SetControl(std::string name, IPropertyData &&val) override;
		virtual bool GetControl(std::string name, IPropertyData &val) override;

		//virtual int SetSensorTrigger(uint8_t level) override;
		//virtual int SetResyncNumber(uint16_t resyncNum) override;
		//virtual int QueryDeviceRole() override;
		//virtual std::string QueryDeviceInfo() override;

		//advanced device controls (on EP0)
		virtual int SetRegisters(uint16_t regaddr[], uint16_t regval[], int num) override;
		virtual int GetRegisters(uint16_t regaddr[], uint16_t regval[], int num) override;
		//single register
		virtual int SetRegister(uint16_t regaddr, uint16_t regval) override;
		virtual int GetRegister(uint16_t regaddr, uint16_t &regval) override;
	protected:
		//enum class ControlCode
		//{
		//	Trigger = 1,
		//	DeviceInfo = 2,
		//	DeviceRole = 3,
		//	RegisterAccess = 4,
		//	Resync = 5,
		//};


		void OnFrame(IVideoStream &parent, std::vector<IVideoFramePtr> &frames);
		//std::shared_ptr<IExtensionAccess> extension;
		ExtensionAccessAdapter extensionAdapter;
		VideoFrameCallbackFn fnCb = nullptr;
		std::vector<VideoFormat> formats;
		int currentFormatIndex = -1;
	private:
		static const int EMBEDDED_LINES = 2;
		void ObtainExtendedLines();
		int header = 0;
		int footer = 0;
		const RegisterMap * registerMap = nullptr;
	public:
		virtual int Flip(bool vertical, bool horizontal) override;
		virtual int GetExposure(uint32_t& val) override;
		virtual int SetExposure(uint32_t val) override;
		virtual int GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB) override;
		virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) override;
		Camaro(std::shared_ptr<IVideoStream> &vs, 
			   std::shared_ptr<IExtensionAccess> &ex,
			CameraProfile &con = CameraProfile::NullObject());
		virtual ~Camaro();
	};
}

