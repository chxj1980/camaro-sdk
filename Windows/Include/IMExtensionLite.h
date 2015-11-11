#pragma once
#include "IExtensionLite.h"
#include "ExtensionUnit.h"

namespace TopGear
{
	namespace Win
	{
		class IMExtensionLite : public IExtensionLite
		{
		public:
			virtual ~IMExtensionLite() = default;
			virtual std::shared_ptr<ExtensionUnit> GetExtensionUnit() const = 0;
		};
	}
}
