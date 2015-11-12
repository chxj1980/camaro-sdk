#include "ExtensionFilterBase.h"
#include <iostream>
#include <ks.h>

using namespace TopGear;
using namespace Win;

ExtensionFilterBase::ExtensionFilterBase(IUnknown* pBase, const std::array<uint8_t, 16> &xucode)
{
	GUID g;
	std::memcpy(&g, xucode.data(), sizeof(GUID));
	pXu = ExtensionUnit::CreateXU(pBase, g);
	if (pXu)
		ObtainInfo();
}

ExtensionFilterBase::~ExtensionFilterBase()
{
}

uint32_t ExtensionFilterBase::GetLen(int index) const
{
	if (index < 1 || index>31)
		return 0;
	return controlLens[index];
}

//HRESULT ExtensionFilterBase::CreateExtensionUnit(IUnknown* pUnkOuter, const GUID &code)
//{
//	//IKsTopologyInfo *pKsTopologyInfo;
//	auto hr = S_OK;
//	try
//	{
//		// pUnkOuter is the unknown associated with the base filter
//		//hr = pUnkOuter->QueryInterface(__uuidof(IKsTopologyInfo),
//		//	reinterpret_cast<void **>(&pKsTopologyInfo));
//		//if (FAILED(hr))
//		//	throw std::system_error(hr, std::generic_category(), "Unable to obtain IKsTopologyInfo");
//		//DWORD dwExtensionNode;
//		//hr = FindExtensionNode(pKsTopologyInfo, dwExtensionNode);
//		//if (hr != S_OK)
//		//	throw std::system_error(hr, std::generic_category(), "Unable to find extension node");
//		//Create extension unit instance by node Id
//		//hr = pKsTopologyInfo->CreateNodeInstance(dwExtensionNode,
//		//	__uuidof(IExtensionUnit),
//		//	reinterpret_cast<void **>(&pExtensionUnit));
//		pXu = ExtensionUnit::CreateXU(pUnkOuter, code);
//		if (pXu == nullptr)
//		{
//			hr = ERROR_SET_NOT_FOUND;
//			throw std::system_error(hr, std::generic_category(), "Unable to create extension node instance");
//		}
//		//System::SafeRelease(&pKsTopologyInfo);
//	}
//	catch (const std::system_error &)
//	{
//		//std::cerr << e.what() << std::endl;
//		//System::SafeRelease(&pKsTopologyInfo);
//	}
//	return hr;
//}

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
