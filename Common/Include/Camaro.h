#pragma once

//#define DEVINFO_AR0134_OLD  "DGFA01"
//#define DEVINFO_AR0134  "DGFA02"
//#define DEVINFO_AR0130  "DGFA03"
#include "IVideoStream.h"
#include "IExtensionAccess.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "CameraBase.h"


namespace TopGear
{
	class Camaro
		: public CameraBase,
		public TopGear::ICameraControl,
		public IDeviceControl
	{
	public:
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		virtual void RegisterFrameCallback(IVideoFrameCallback* pCB) override;

		virtual bool StartStream(int formatIndex) override;
		virtual bool StopStream() override;

		virtual int SetSensorTrigger(uint8_t level) override;
		virtual int SetResyncNumber(uint16_t resyncNum) override;
		virtual int QueryDeviceRole() override;
		virtual std::string QueryDeviceInfo() override;
	protected:
		enum class ControlCode
		{
			Trigger = 1,
			DeviceInfo = 2,
			DeviceRole = 3,
			RegisterAccess = 4,
			Resync = 5,
		};

		//enum class DeviceType
		//{
		//	AR0134_OLD,
		//	AR0134,
		//	AR0130,
		//	UNKNOWN_DEV
		//};

		//advanced device controls (on EP0)
		virtual int SetRegisters(uint16_t regaddr[], uint16_t regval[], int num);
		virtual int GetRegisters(uint16_t regaddr[], uint16_t regval[], int num);
		//single register
		virtual int SetRegister(uint16_t regaddr, uint16_t regval);
		virtual int GetRegister(uint16_t regaddr, uint16_t &regval);

		
		void OnFrame(std::vector<IVideoFrameRef> &frames);
		std::shared_ptr<IExtensionAccess> extension;
		VideoFrameCallbackFn fnCb = nullptr;
		IVideoFrameCallback *pCbobj = nullptr;
	private:
		static const int EMBEDDED_LINES = 2;
		void ObtainExtendedLines();
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

