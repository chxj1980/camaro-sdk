#pragma once
#include "IExtensionAccess.h"
//#include "IExtensionUnit.h"
#include "IMExtensionLite.h"

namespace TopGear
{
	namespace Win
	{
		class ExtensionAccess : public IExtensionAccess
		{
		public:
			virtual int SetProperty(int index, const uint8_t* data, size_t size) override;
			virtual std::unique_ptr<uint8_t[]> GetProperty(int index, int &len) override;
			explicit ExtensionAccess(std::shared_ptr<IMExtensionLite> &validator);
			virtual ~ExtensionAccess();
		private:
			std::shared_ptr<ExtensionUnit> pXu;
			std::shared_ptr<IMExtensionLite> extensionAgent;
		};
	}
}

