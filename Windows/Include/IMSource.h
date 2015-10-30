#pragma once
#include <utility>
#include <mfidl.h>

namespace TopGear
{
	namespace Win
	{
		typedef std::pair<IMFActivate *, IMFMediaSource *> SourcePair;

		class IMSource
		{
		public:
			virtual ~IMSource() = default;
			virtual IMFActivate *GetActivator() = 0;
			virtual IMFMediaSource *GetSource() = 0;
		};
	}
}
