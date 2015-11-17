#pragma once

#include "ExtensionFilterBase.h"
#include "ExtensionRepository.h"

namespace TopGear
{
	namespace Win
	{
		class EtronExtensionFilter : public ExtensionFilterBase
		{
		public:
			explicit EtronExtensionFilter(std::shared_ptr<IGenericVCDevice> &device)
				: ExtensionFilterBase(device, ExtensionRepository::EtronXuCode)
			{}
			virtual ~EtronExtensionFilter() {}
			virtual std::string GetDeviceInfo() override { return{}; }
		};
	}
}