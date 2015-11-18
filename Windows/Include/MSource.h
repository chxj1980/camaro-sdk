#pragma once
#include <utility>
#include <mfidl.h>
#include "System.h"
#include "MFHelper.h"
#include "IGenericVCDevice.h"

namespace TopGear
{
	namespace Win
	{
		class MSource final
			: public ISource
		{
		public:
			explicit MSource(SourcePair &sp)
				: pActivate(sp.first), pSource(sp.second)
			{
				
			}
			explicit MSource(SourcePair &&sp)
				: pActivate(sp.first), pSource(sp.second)
			{

			}
			virtual ~MSource()
			{
				System::SafeRelease(&pActivate);
				System::SafeRelease(&pSource);
			}
			IMFActivate *GetActivator() const { return pActivate; }
			IMFMediaSource *GetMediaSource() const { return pSource; }
		protected:
			IMFActivate *pActivate;
			IMFMediaSource *pSource;
			
		};
	}
}
