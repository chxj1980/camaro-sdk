#pragma once
#include "IExtensionAccess.h"
#include "ExtensionFilterBase.h"

namespace TopGear
{
    namespace Linux
	{
		class ExtensionAccess : public IExtensionAccess
		{
		public:
			virtual int SetProperty(int index, const uint8_t* data, size_t size) override;
            virtual std::unique_ptr<uint8_t[]> GetProperty(int index, int &len, bool dynamicLen = false) override;
            explicit ExtensionAccess(int dev, std::shared_ptr<ExtensionFilterBase> &validator);
            virtual ~ExtensionAccess();
        private:
            int handle;
            std::shared_ptr<ExtensionFilterBase> extensionAgent;
            int unitId;
        };
    }
}

