#pragma once
#include <memory>
#include "ExtensionInfo.h"
#include "IExtensionLite.h"

namespace TopGear
{
    namespace Linux
	{
        class ILExtensionLite : public IExtensionLite
		{
		public:
            virtual ~ILExtensionLite() = default;
            virtual int GetExtensionUnit() const = 0;
		};
	}
}
