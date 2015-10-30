#pragma once
#include "IExtensionUnit.h"
#include "IExtensionLite.h"

namespace TopGear
{
	namespace Win
	{
		class IMExtensionLite : public IExtensionLite
		{
		public:
			virtual ~IMExtensionLite() = default;
			virtual IExtensionUnit *GetExtensionUnit(bool addRef = false) const = 0;
		};
	}
}
