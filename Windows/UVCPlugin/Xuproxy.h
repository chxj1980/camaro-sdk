#pragma once

#include "stdafx.h"


#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>
#include <ksmedia.h>

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#endif

#include "UVCPlugin_i.h"


// {497515BC-99DE-4A60-7590-C4542D298DC2}
DEFINE_GUID(CLSID_ExtensionUnit, 0x497515BC, 0x99DE, 0x4A60, 0x75, 0x90, 0xc4, 0x54, 0x2d, 0x29, 0x8d, 0xc2);


class CNodeControl : public IKsNodeControl
{
public:
	virtual ~CNodeControl()
	{
	}

	STDMETHOD(put_NodeId) (DWORD dwNodeId) override;
	STDMETHOD(put_KsControl) (PVOID pKsControl) override;

	DWORD m_dwNodeId;
	ATL::CComPtr<IKsControl> m_pKsControl;
};

class CExtension :
	public IExtensionUnit,
	public ATL::CComObjectRootEx<ATL::CComObjectThreadModel>,
	public ATL::CComCoClass<CExtension, &CLSID_ExtensionUnit>,
	public CNodeControl
{
public:

	CExtension();
	//ReSharper disable once CppHidingFunction
	STDMETHOD(FinalConstruct)();

	BEGIN_COM_MAP(CExtension)
		COM_INTERFACE_ENTRY(IKsNodeControl)
		COM_INTERFACE_ENTRY(IExtensionUnit)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_NO_REGISTRY()
	DECLARE_ONLY_AGGREGATABLE(CExtension)

	// IExtensionUnit
	STDMETHOD(get_Info)(
		ULONG ulSize,
		BYTE pInfo[]) override;
	STDMETHOD(get_InfoSize)(
		ULONG *pulSize) override;
	STDMETHOD(get_PropertySize)(
		ULONG PropertyId,
		ULONG *pulSize) override;
	STDMETHOD(get_Property)(
		ULONG PropertyId,
		ULONG ulSize,
		BYTE pValue[]) override;
	STDMETHOD(put_Property)(
		ULONG PropertyId,
		ULONG ulSize,
		BYTE pValue[]) override;
	STDMETHOD(get_PropertyRange)(
		ULONG PropertyId,
		ULONG ulSize,
		BYTE pMin[],
		BYTE pMax[],
		BYTE pSteppingDelta[],
		BYTE pDefault[]) override;
};


OBJECT_ENTRY_AUTO(CLSID_ExtensionUnit, CExtension)


#define STATIC_PROPSETID_VIDCAP_EXTENSION_UNIT   \
0xffffffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

DEFINE_GUIDSTRUCT("ffffffff-ffff-ffff-ffff-ffffffffffff", PROPSETID_VIDCAP_EXTENSION_UNIT);
#define PROPSETID_VIDCAP_EXTENSION_UNIT DEFINE_GUIDNAMED(PROPSETID_VIDCAP_EXTENSION_UNIT)

