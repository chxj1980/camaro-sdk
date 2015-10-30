#include "stdafx.h"
#include "Xuproxy.h"

STDMETHODIMP
CNodeControl::put_NodeId(
	DWORD dwNodeId)
{
	m_dwNodeId = dwNodeId;
	return S_OK;
}

STDMETHODIMP
CNodeControl::put_KsControl(
	PVOID pKsControl)
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	IKsControl *pIKsControl;

	if (!pKsControl) return E_POINTER;
	pIKsControl = reinterpret_cast<IKsControl *>(pKsControl);

	if (m_pKsControl) m_pKsControl.Release();
	hr = pIKsControl->QueryInterface(__uuidof(IKsControl),
		reinterpret_cast<void **>(&m_pKsControl));

	return hr;
}

CExtension::CExtension()
{
	m_pKsControl = nullptr;
}

STDMETHODIMP
CExtension::FinalConstruct()
{
	if (m_pOuterUnknown == nullptr) return E_FAIL;
	return S_OK;
}

STDMETHODIMP
CExtension::get_InfoSize(
	ULONG *pulSize)
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	ULONG ulBytesReturned;
	KSP_NODE ExtensionProp;

	if (!pulSize) return E_POINTER;

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = m_dwNodeId;

	hr = m_pKsControl->KsProperty(
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


STDMETHODIMP
CExtension::get_Info(
	ULONG ulSize,
	BYTE pInfo[])
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = m_dwNodeId;
	
	pInfo[0] = static_cast<BYTE>(m_dwNodeId);
	hr = m_pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pInfo+1),
		ulSize-1,
		&ulBytesReturned);

	return hr;
}


STDMETHODIMP
CExtension::get_PropertySize(
	ULONG PropertyId,
	ULONG *pulSize)
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	ULONG ulBytesReturned;
	KSP_NODE ExtensionProp;

	if (!pulSize) return E_POINTER;

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = m_dwNodeId;

	hr = m_pKsControl->KsProperty(
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

STDMETHODIMP
CExtension::get_Property(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pValue[])
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET |
		KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = m_dwNodeId;

	hr = m_pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pValue),
		ulSize,
		&ulBytesReturned);

	return hr;
}

STDMETHODIMP
CExtension::put_Property(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pValue[])
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto hr = S_OK;
	KSP_NODE ExtensionProp;
	ULONG ulBytesReturned;

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = PropertyId;
	ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET |
		KSPROPERTY_TYPE_TOPOLOGY;
	ExtensionProp.NodeId = m_dwNodeId;

	hr = m_pKsControl->KsProperty(
		reinterpret_cast<PKSPROPERTY>(&ExtensionProp),
		sizeof(ExtensionProp),
		PVOID(pValue),
		ulSize,
		&ulBytesReturned);

	return hr;
}

STDMETHODIMP
CExtension::get_PropertyRange(
	ULONG PropertyId,
	ULONG ulSize,
	BYTE pMin[],
	BYTE pMax[],
	BYTE pSteppingDelta[],
	BYTE pDefault[])
{
	// IHV may add code here, current stub just returns S_OK
	auto hr = S_OK;
	return hr;
}