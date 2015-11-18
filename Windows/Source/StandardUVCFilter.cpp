#include "StandardUVCFilter.h"
#include "ExtensionRepository.h"
#include "MSource.h"
#include "ExtensionUnit.h"

using namespace TopGear;
using namespace Win;

StandardUVCFilter::StandardUVCFilter(std::shared_ptr<IGenericVCDevice> &device)
	:isValid(true)
{
	auto source = std::dynamic_pointer_cast<MSource>(device->GetSource());
	if (source == nullptr)
		return;
	IUnknown* pUnk = source->GetMediaSource();
	std::shared_ptr<ExtensionUnit> xu;
	GUID g;
	for (auto &xucode : ExtensionRepository::Inventory)
	{
		std::memcpy(&g, xucode.data(), sizeof(GUID));
		xu = ExtensionUnit::CreateXU(pUnk, g);
		if (xu)
		{
			isValid = false;
			break;
		}
	}
}
