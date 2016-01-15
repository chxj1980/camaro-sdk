#include "ExtensionFilterBase.h"
#include "MSource.h"

using namespace TopGear;
using namespace Win;

ExtensionFilterBase::ExtensionFilterBase(std::shared_ptr<IGenericVCDevice> &device, const std::array<uint8_t, 16> &xucode)
{
	auto source = std::dynamic_pointer_cast<MSource>(device->GetSource());
	if (source == nullptr)
		return;
	IUnknown* pUnk = source->GetMediaSource();
	GUID g;
	std::memcpy(&g, xucode.data(), sizeof(GUID));
	pXu = ExtensionUnit::CreateXU(pUnk, g);
	if (pXu)
		ObtainInfo();
}

ExtensionFilterBase::~ExtensionFilterBase()
{
}

uint32_t ExtensionFilterBase::GetLen(int index, bool live) const
{
	if (index < 1 || index>31)
		return 0;
	if (live)
	{
		ULONG ulSize;
		return (pXu->get_PropertySize(index, &ulSize) == S_OK) ? ulSize : 0;
	}
	return controlLens[index];
}

bool ExtensionFilterBase::ObtainInfo()
{
	ULONG ulSize;
	auto hr = pXu->get_InfoSize(&ulSize);
	if (FAILED(hr))
		return false;
	std::unique_ptr<uint8_t[]> info(new uint8_t[ulSize]{ 0 });
	hr = pXu->get_Info(ulSize, info.get());
	if (FAILED(hr))
		return false;
	pInfo = ExtensionInfo::Parse(info.get(), ulSize);
	if (pInfo == nullptr)
		return false;
	auto count = 0;
	for (auto i = 0; i < pInfo->ControlSize<<3; ++i)
	{
		// ReSharper disable once CppRedundantParentheses
		if ((pInfo->Controls[i >> 3] & 1 << (i & 0x7)) == 0)
			continue;
		if (pXu->get_PropertySize(i+1, &ulSize) == S_OK)
			controlLens[i+1] = ulSize;
		if (++count >= pInfo->NumControls)
			break;
	}
	return true;
}
