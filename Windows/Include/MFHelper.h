#pragma once

#include <mfapi.h>
#include <vector>
#include <chrono>
#include "IMSource.h"


namespace TopGear
{
	namespace Win
	{
		class MFHelper
		{
		public:
			static HRESULT EnumVideoDeviceSources(std::vector<SourcePair> &sources, std::chrono::milliseconds initialTime);
			static HRESULT GetDefaultStride(IMFMediaType *pType, LONG &Stride);
			static RECT CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR);
			static RECT LetterBoxRect(const RECT& rcSrc, const RECT& rcDst);
			static HRESULT GetAttributeSize(IMFMediaType *pType, int &width, int &height);
			static HRESULT GetAttributePixelRatio(IMFMediaType *pType, int &numerator, int &denominator);
			static HRESULT GetAttributeFrameRate(IMFMediaType *pType, int &numerator, int &denominator);
		private:
			static HRESULT MfGetAttribute2Uint32AsUint64(IMFMediaType *pType, GUID guidKey, int &punHigh32, int &punLow32);
			MFHelper() {}
			~MFHelper() {}
		};
	}
}

