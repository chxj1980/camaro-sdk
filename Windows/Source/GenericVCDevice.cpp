#include "GenericVCDevice.h"
#include "System.h"

using namespace TopGear;
using namespace Win;

bool GenericVCDevice::GetAllocatedString(const GUID& guidCode, std::string &val) const
{
	WCHAR *pwsz;
	UINT32 cch;
	auto hr = pActivate->GetAllocatedString(guidCode, &pwsz, &cch);
	if (SUCCEEDED(hr))
	{
		std::wstring wstr(pwsz, cch);
		val = std::string(wstr.begin(), wstr.end());
	}
	else
	{
		val.clear();
	}
	CoTaskMemFree(pwsz);
	return SUCCEEDED(hr);
}

std::string GenericVCDevice::GetSymbolicLink()
{
	if (symbolicLink.empty())
		GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolicLink);
	return symbolicLink;
}

std::string GenericVCDevice::GetFriendlyName()
{
	if (name.empty())
		GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name);
	return name;
}

GenericVCDevice::GenericVCDevice(IMFActivate *pAct, IMFMediaSource *pSrc)
	: pActivate(pAct),
	pSource(pSrc)
{
}

GenericVCDevice::~GenericVCDevice()
{
	System::SafeRelease(&pActivate);
	System::SafeRelease(&pSource);
}