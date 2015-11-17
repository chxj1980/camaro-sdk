#include "DGExtensionFilter.h"
#include "ExtensionRepository.h"

using namespace TopGear;
using namespace Win;


DGExtensionFilter::DGExtensionFilter(std::shared_ptr<IGenericVCDevice> &device)
	: ExtensionFilterBase(device, ExtensionRepository::DGXuCode)
{
	
}

std::string DGExtensionFilter::GetDeviceInfo()
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