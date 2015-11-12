#include "StandardUVCFilter.h"
#include "ExtensionFilterBase.h"
#include "ExtensionRepository.h"

using namespace TopGear::Win;

StandardUVCFilter::StandardUVCFilter(IUnknown* pBase)
	:isValid(true)
{
	std::shared_ptr<ExtensionUnit> xu;
	GUID g;
	for (auto &xucode : ExtensionRepository::Inventory)
	{
		std::memcpy(&g, xucode.data(), sizeof(GUID));
		xu = ExtensionUnit::CreateXU(pBase, g);
		if (xu)
		{
			isValid = false;
			break;
		}
	}
}

StandardUVCFilter::~StandardUVCFilter()
{
}
