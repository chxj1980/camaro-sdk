#pragma once
#include "ExtensionInfo.h"
#include "IValidation.h"

namespace TopGear
{
	class IExtensionLite : public IValidation
	{
	public:
		virtual ~IExtensionLite() = default;
		virtual const std::shared_ptr<ExtensionInfo> &GetExtensionInfo() const = 0;
		virtual uint32_t GetLen(int index, bool live = false) const = 0;
		virtual std::string GetDeviceInfo() = 0;
	};
}