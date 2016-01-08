#pragma once
// ReSharper disable CppUnusedIncludeDirective
#include <memory>
#include <atlbase.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include <vidcap.h>
// ReSharper restore CppUnusedIncludeDirective


//#ifndef INITGUID
//#define INITGUID
//#include <guiddef.h>
//#undef INITGUID
//#endif


namespace TopGear
{
	namespace Win
	{
		class ExtensionUnit
		{
		public:
			static std::shared_ptr<ExtensionUnit> CreateXU(IUnknown* pUnkOuter, const GUID &xCode);
			~ExtensionUnit();
			HRESULT get_InfoSize(
				/* [out] */ ULONG *pulSize) const;

			HRESULT get_Info(
				/* [in] */ ULONG ulSize,
				/* [size_is][out][in] */ BYTE pInfo[]) const;

			HRESULT get_PropertySize(
				/* [in] */ ULONG PropertyId,
				/* [out] */ ULONG *pulSize) const;

			HRESULT get_Property(
				/* [in] */ ULONG PropertyId,
				/* [in] */ ULONG ulSize,
				/* [size_is][out][in] */ BYTE pValue[]) const;

			HRESULT put_Property(
				/* [in] */ ULONG PropertyId,
				/* [in] */ ULONG ulSize,
				/* [size_is][out][in] */ BYTE pValue[]) const;

			HRESULT get_PropertyRange(
				/* [in] */ ULONG PropertyId,
				/* [in] */ ULONG ulSize,
				/* [size_is][out][in] */ BYTE pMin[],
				/* [size_is][out][in] */ BYTE pMax[],
				/* [size_is][out][in] */ BYTE pSteppingDelta[],
				/* [size_is][out][in] */ BYTE pDefault[]) const;
		private:
			IKsControl *pKsControl;
			DWORD dwNodeId;
			GUID gCode;
			ExtensionUnit(IKsControl *control, DWORD nodeId, const GUID& xCode);
			static HRESULT ExtensionUnit::FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, DWORD& nodeId);
		};
	}
}