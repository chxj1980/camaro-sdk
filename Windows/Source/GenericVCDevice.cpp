#include "GenericVCDevice.h"
#include <mfidl.h>
#include "MFHelper.h"
#include "MSource.h"

using namespace TopGear;
using namespace Win;


std::string GenericVCDevice::GetSymbolicLink()
{
	if (symbolicLink.empty())
	{
		auto activator = std::dynamic_pointer_cast<MSource>(source);
		if (activator!=nullptr)
			MFHelper::GetAllocatedString(activator->GetActivator(), MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolicLink);
	}
	return symbolicLink;
}

std::string GenericVCDevice::GetFriendlyName()
{
	if (name.empty())
	{
		auto activator = std::dynamic_pointer_cast<MSource>(source);
		if (activator != nullptr)
			MFHelper::GetAllocatedString(activator->GetActivator(), MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name);
	}
	return name;
}

GenericVCDevice::GenericVCDevice(std::shared_ptr<ISource> &vsource)
	: source(vsource)
{
}

GenericVCDevice::~GenericVCDevice()
{
}