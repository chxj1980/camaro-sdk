#include "ExtensionUnit.h"
#include "System.h"

using namespace TopGear;
using namespace Win;

std::shared_ptr<ExtensionUnit> ExtensionUnit::CreateXU(IUnknown* pUnkOuter, const GUID& xCode)
{
	IKsTopologyInfo *pKsTopologyInfo;
	
	auto hr = pUnkOuter->QueryInterface(__uuidof(IKsTopologyInfo),
		reinterpret_cast<void **>(&pKsTopologyInfo));
	if (FAILED(hr))
	{
		//throw std::system_error(hr, std::generic_category(), "Unable to obtain IKsTopologyInfo");
		System::SafeRelease(&pKsTopologyInfo);
		return{};
	}
	DWORD dwExtensionNode;
	hr = FindExtensionNode(pKsTopologyInfo, dwExtensionNode);
	if (hr != S_OK)
	{
		//throw std::system_error(hr, std::generic_category(), "Unable to find extension node");
		System::SafeRelease(&pKsTopologyInfo);
		return{};
	}
	System::SafeRelease(&pKsTopologyInfo);

	IKsControl *control;
	hr = pUnkOuter->QueryInterface(IID_PPV_ARGS(&control));
	if (hr != S_OK)
		return{};
	std::shared_ptr<ExtensionUnit> xu(new ExtensionUnit(control, dwExtensionNode, xCode));
	ULONG size;
	hr = xu->get_InfoSize(&size);
	if (hr != S_OK)
		return{};
	std::unique_ptr<uint8_t[]> info(new uint8_t[size]{ 0 });
	hr = xu->get_Info(size, info.get());
	if (hr != S_OK)
		return{};
	if (std::memcmp(&info[1], &xCode, sizeof(GUID)) == 0)
		return xu;
	return{};
}

HRESULT ExtensionUnit::FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, DWORD& nodeId)
{
	DWORD numberOfNodes;
	auto hResult = pIksTopologyInfo->get_NumNodes(&numberOfNodes);
	if (SUCCEEDED(hResult))
	{
		GUID nodeGuid;
		for (auto i = 0u; i < numberOfNodes; i++)
			if (SUCCEEDED(pIksTopologyInfo->get_NodeType(i, &nodeGuid)))
			{
				if (IsEqualGUID(KSNODETYPE_DEV_SPECIFIC, nodeGuid))
				{	// Found the extension node
					nodeId = i;
					return S_OK;
				}
			}
		// Did not find the node
		hResult = S_FALSE;
	}
	return hResult;
}

ExtensionUnit::~ExtensionUnit()
{
	System::SafeRelease(&pKsControl);
}

HRESULT ExtensionUnit::get_InfoSize(ULONG *pulSize) const
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	ULONG ulBytesReturned;
	KSP_NODE ExtensionProp;

	if (!pulSize) return E_POINTER;

	ExtensionProp.Property.Set = gCode;
	ExtensionProp.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = dwNodeId;

	hr = pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		nullptr,
		0,
		&ulBytesReturned);

	if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
	{
		*pulSize = ulBytesReturned+1;
		hr = S_OK;
	}

	return hr;
}


HRESULT ExtensionUnit::get_Info(
	ULONG ulSize,
	BYTE pInfo[]) const
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = gCode;
	ExtensionProp.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = dwNodeId;
	
	pInfo[0] = static_cast<BYTE>(dwNodeId);
	hr = pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pInfo+1),
		ulSize-1,
		&ulBytesReturned);

	return hr;
}


HRESULT ExtensionUnit::get_PropertySize(
	ULONG PropertyId,
	ULONG *pulSize) const
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	ULONG ulBytesReturned;
	KSP_NODE ExtensionProp;

	if (!pulSize) return E_POINTER;

	ExtensionProp.Property.Set = gCode;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = dwNodeId;

	hr = pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		nullptr,
		0,
		&ulBytesReturned);

	if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
	{
		*pulSize = ulBytesReturned;
		hr = S_OK;
	}

	return hr;
}

HRESULT ExtensionUnit::get_Property(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pValue[]) const
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = gCode;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET |
		KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = dwNodeId;

	hr = pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pValue),
		ulSize,
		&ulBytesReturned);

	return hr;
}

HRESULT ExtensionUnit::put_Property(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pValue[]) const
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = gCode;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET |
		KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = dwNodeId;

	hr = pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pValue),
		ulSize,
		&ulBytesReturned);

	return hr;
}

HRESULT ExtensionUnit::get_PropertyRange(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pMin[],
	BYTE pMax[],
	BYTE pSteppingDelta[],
	BYTE pDefault[]) const
{
	// IHV may add code here, current stub just returns S_OK
	return S_OK;
}

ExtensionUnit::ExtensionUnit(IKsControl* control, DWORD nodeId, const GUID& xCode)
	:pKsControl(control), dwNodeId(nodeId),gCode(xCode)
{
}