#pragma once
#include <vector>
#include <mfidl.h>
#include "IMSource.h"

namespace TopGear
{
	namespace Win
	{
		class GenericVCDevice : public IMSource
		{
		public:
			virtual IMFActivate* GetActivator() override { return pActivate; }
			virtual IMFMediaSource* GetSource() override { return pSource; }
			virtual std::string GetSymbolicLink() override;
			virtual std::string GetFriendlyName() override;
			virtual std::string GetDeviceInfo() override { return ""; }
			GenericVCDevice(IMFActivate *pAct, IMFMediaSource *pSrc);
			virtual ~GenericVCDevice();
		private:
			bool GetAllocatedString(const GUID& guidCode, std::string &val) const;
		protected:
			IMFActivate *pActivate;
			IMFMediaSource *pSource;
			std::string symbolicLink;
			std::string name;
		};
	}
}
