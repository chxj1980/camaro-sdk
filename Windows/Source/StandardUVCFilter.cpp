#include "StandardUVCFilter.h"
#include "GeneralExtensionFilter.h"
#include "System.h"

using namespace TopGear::Win;

StandardUVCFilter::StandardUVCFilter(IUnknown* pBase)
{
	IExtensionUnit *pXu = nullptr;
	isValid = FAILED(GeneralExtensionFilter::CreateExtensionUnit(pBase, &pXu));
	System::SafeRelease(&pXu);
}

StandardUVCFilter::~StandardUVCFilter()
{
}
