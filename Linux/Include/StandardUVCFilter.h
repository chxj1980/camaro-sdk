#pragma once
#include "IGenericVCDevice.h"

namespace TopGear
{
    namespace Linux
	{
		class StandardUVCFilter : public IValidation
		{
		public:
			virtual bool IsValid() const override { return isValid; }
            explicit StandardUVCFilter(int dev);
			virtual ~StandardUVCFilter();
		private:
			bool isValid;
		};
	}
}

