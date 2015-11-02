#include "MFHelper.h"
#include <cstdint>
#include "System.h"
#include <iostream>
#include <thread>


using namespace TopGear;
using namespace Win;

HRESULT MFHelper::EnumVideoDeviceSources(std::vector<SourcePair>& sources, std::chrono::milliseconds initialTime)
{
	IMFAttributes *pAttributes = nullptr;
	IMFActivate **ppDevices = nullptr;
	UINT32 count;
	HRESULT hr;
	try
	{
		// Create an attribute store to specify the enumeration parameters.
		hr = MFCreateAttributes(&pAttributes, 1);
		if (FAILED(hr))
			throw std::system_error(hr, std::generic_category(), "MFCreateAttributes failed");

		// Source type: video capture devices
		hr = pAttributes->SetGUID(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
			);
		if (FAILED(hr))
			throw std::system_error(hr, std::generic_category(), "Unable to set video source type");

		// Enumerate devices.

		hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
		if (FAILED(hr))
			throw std::system_error(hr, std::generic_category(), "Unable to enumerate devices");

		if (count == 0)
		{
			hr = E_FAIL;
			throw std::system_error(hr, std::generic_category(), "No available device");
		}

		for (auto i = 0u; i < count; ++i)
		{
			IMFMediaSource *pSource;
			// Create the media source object.
			auto result = ppDevices[i]->ActivateObject(IID_PPV_ARGS(&pSource));
			if (SUCCEEDED(result))
				sources.push_back(std::make_pair(ppDevices[i], pSource));
			else
			{
				System::SafeRelease(&ppDevices[i]);
				System::SafeRelease(&pSource);
			}
		}
		std::this_thread::sleep_for(initialTime); //Important! wait for device initialization
		System::SafeRelease(&pAttributes);
		CoTaskMemFree(ppDevices);
	}
	catch (const std::system_error &e)
	{
		std::cerr << e.what() << std::endl;
		System::SafeRelease(&pAttributes);
		for (auto i = 0u; i < count; i++)
			System::SafeRelease(&ppDevices[i]);
		CoTaskMemFree(ppDevices);
		sources.clear();
		throw;
	}
	return hr;
}

HRESULT MFHelper::GetDefaultStride(IMFMediaType *pType, LONG &stride)
{
	LONG lStride = 0;

	// Try to get the default stride from the media type.
	auto hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, reinterpret_cast<UINT32*>(&lStride));
	if (FAILED(hr))
	{ 
		// Attribute not set. Try to calculate the default stride.
		auto subtype = GUID_NULL;

		UINT32 width = 0;
		UINT32 height = 0;

		// Get the subtype and the image size.
		hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
		if (SUCCEEDED(hr))
		{
			hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
		}
		if (SUCCEEDED(hr))
		{
			hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &lStride);
		}

		// Set the attribute for later reference.
		if (SUCCEEDED(hr))
		{
			(void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
		}
		else
		{
			if (strncmp(reinterpret_cast<char *>(&subtype.Data1), "BA12", 4) == 0)
			{
				lStride = width << 1;
				(void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
				hr = S_OK;
			}
		}
	}
		
	stride = SUCCEEDED(hr) ? lStride : 0;
	return hr;
}

//-----------------------------------------------------------------------------
// CorrectAspectRatio
//
// Converts a rectangle from the source's pixel aspect ratio (PAR) to 1:1 PAR.
// Returns the corrected rectangle.
//
// For example, a 720 x 486 rect with a PAR of 9:10, when converted to 1x1 PAR,  
// is stretched to 720 x 540. 
//-----------------------------------------------------------------------------

RECT MFHelper::CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR)
{
	// Start with a rectangle the same size as src, but offset to the origin (0,0).
	RECT rc = { 0, 0, src.right - src.left, src.bottom - src.top };

	if ((srcPAR.Numerator != 1) || (srcPAR.Denominator != 1))
	{
		// Correct for the source's PAR.

		if (srcPAR.Numerator > srcPAR.Denominator)
		{
			// The source has "wide" pixels, so stretch the width.
			rc.right = MulDiv(rc.right, srcPAR.Numerator, srcPAR.Denominator);
		}
		else if (srcPAR.Numerator < srcPAR.Denominator)
		{
			// The source has "tall" pixels, so stretch the height.
			rc.bottom = MulDiv(rc.bottom, srcPAR.Denominator, srcPAR.Numerator);
		}
		// else: PAR is 1:1, which is a no-op.
	}
	return rc;
}

//-------------------------------------------------------------------
// LetterBoxDstRect
//
// Takes a src rectangle and constructs the largest possible 
// destination rectangle within the specifed destination rectangle 
// such thatthe video maintains its current shape.
//
// This function assumes that pels are the same shape within both the 
// source and destination rectangles.
//
//-------------------------------------------------------------------

RECT MFHelper::LetterBoxRect(const RECT& rcSrc, const RECT& rcDst)
{
	// figure out src/dest scale ratios
	int iSrcWidth = rcSrc.right - rcSrc.left;
	int iSrcHeight = rcSrc.bottom - rcSrc.top;

	int iDstWidth = rcDst.right - rcDst.left;;
	int iDstHeight = rcDst.bottom - rcDst.top;

	int iDstLBWidth;
	int iDstLBHeight;

	if (MulDiv(iSrcWidth, iDstHeight, iSrcHeight) <= iDstWidth) {

		// Column letter boxing ("pillar box")

		iDstLBWidth = MulDiv(iDstHeight, iSrcWidth, iSrcHeight);
		iDstLBHeight = iDstHeight;
	}
	else {

		// Row letter boxing.

		iDstLBWidth = iDstWidth;
		iDstLBHeight = MulDiv(iDstWidth, iSrcHeight, iSrcWidth);
	}


	// Create a centered rectangle within the current destination rect

	RECT rc;

	auto left = rcDst.left + ((iDstWidth - iDstLBWidth) / 2);
	auto top = rcDst.top + ((iDstHeight - iDstLBHeight) / 2);

	SetRect(&rc, left, top, left + iDstLBWidth, top + iDstLBHeight);

	return rc;
}

HRESULT MFHelper::GetAttributeSize(IMFMediaType *pType, int &width, int &height)
{
	return MfGetAttribute2Uint32AsUint64(pType, MF_MT_FRAME_SIZE, width, height);
}

HRESULT MFHelper::GetAttributePixelRatio(IMFMediaType *pType, int &numerator, int &denominator)
{
	return MfGetAttribute2Uint32AsUint64(pType, MF_MT_PIXEL_ASPECT_RATIO, numerator, denominator);
}

HRESULT MFHelper::GetAttributeFrameRate(IMFMediaType *pType, int &numerator, int &denominator)
{
	return MfGetAttribute2Uint32AsUint64(pType, MF_MT_FRAME_RATE, numerator, denominator);
}

HRESULT MFHelper::MfGetAttribute2Uint32AsUint64(IMFMediaType *pType, GUID guidKey, int &punHigh32, int &punLow32)
{
	uint64_t attrValue;
	auto hr = pType->GetUINT64(guidKey, &attrValue);

	if (SUCCEEDED(hr))
	{
		punLow32 = static_cast<int>(attrValue);
		punHigh32 = static_cast<int>(attrValue >> 32);
	}
	else
	{
		punLow32 = 0;
		punHigh32 = 0;
	}

	return hr;
}
