#pragma once
#include <utility>
#include <mfidl.h>
#include "IGenericVCDevice.h"

namespace TopGear
{
	namespace Win
	{
		typedef std::pair<IMFActivate *, IMFMediaSource *> SourcePair;

		class IMSource : public IGenericVCDevice
		{
		public:
			virtual ~IMSource() = default;
			virtual IMFActivate *GetActivator() = 0;
			virtual IMFMediaSource *GetSource() = 0;
		};
	}
}
