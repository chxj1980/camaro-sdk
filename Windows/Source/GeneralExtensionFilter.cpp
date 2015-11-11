#include "GeneralExtensionFilter.h"
#include <iostream>
#include <ks.h>

using namespace TopGear;
using namespace Win;

GeneralExtensionFilter::GeneralExtensionFilter(IUnknown* pBase)
{
	if (CreateExtensionUnit(pBase)==S_OK)
		ObtainInfo();
}

GeneralExtensionFilter::~GeneralExtensionFilter()
{
}

std::string GeneralExtensionFilter::GetDeviceInfo()
{
	if (deviceInfo.empty())
	{
		std::unique_ptr<uint8_t[]> data(new uint8_t[controlLens[DeviceInfoCode]]{ 0 });
		auto res = pXu->get_Property(DeviceInfoCode, controlLens[DeviceInfoCode], data.get());

		if (FAILED(res))
		{
			printf("Unable to get property value\n");
			deviceInfo.clear();
		}
		else
			deviceInfo = std::string(reinterpret_cast<char *>(data.get()));
	}
	return deviceInfo;
}

uint32_t GeneralExtensionFilter::GetLen(int index) const
{
	if (index < 1 || index>31)
		return 0;
	return controlLens[index];
}

HRESULT GeneralExtensionFilter::CreateExtensionUnit(IUnknown* pUnkOuter)
{
	//IKsTopologyInfo *pKsTopologyInfo;
	auto hr = S_OK;
	try
	{
		// pUnkOuter is the unknown associated with the base filter
		//hr = pUnkOuter->QueryInterface(__uuidof(IKsTopologyInfo),
		//	reinterpret_cast<void **>(&pKsTopologyInfo));
		//if (FAILED(hr))
		//	throw std::system_error(hr, std::generic_category(), "Unable to obtain IKsTopologyInfo");
		//DWORD dwExtensionNode;
		//hr = FindExtensionNode(pKsTopologyInfo, dwExtensionNode);
		//if (hr != S_OK)
		//	throw std::system_error(hr, std::generic_category(), "Unable to find extension node");
		//Create extension unit instance by node Id
		//hr = pKsTopologyInfo->CreateNodeInstance(dwExtensionNode,
		//	__uuidof(IExtensionUnit),
		//	reinterpret_cast<void **>(&pExtensionUnit));
		pXu = ExtensionUnit::CreateXU(pUnkOuter, PROPSETID_VIDCAP_EXTENSION_UNIT);
		if (pXu == nullptr)
		{
			hr = ERROR_SET_NOT_FOUND;
			throw std::system_error(hr, std::generic_category(), "Unable to create extension node instance");
		}
		//System::SafeRelease(&pKsTopologyInfo);
	}
	catch (const std::system_error &)
	{
		//std::cerr << e.what() << std::endl;
		//System::SafeRelease(&pKsTopologyInfo);
	}
	return hr;
}

bool GeneralExtensionFilter::ObtainInfo()
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
