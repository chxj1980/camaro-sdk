#pragma once
#include "IGenericVCDevice.h"
#include <unknwn.h>

namespace TopGear
{
	namespace Win
	{
		class StandardUVCFilter : public IValidation
		{
		public:
			virtual bool IsValid() const override { return isValid; }
			explicit StandardUVCFilter(IUnknown *pBase);
			virtual ~StandardUVCFilter();
		private:
			bool isValid;
		};
	}
}

