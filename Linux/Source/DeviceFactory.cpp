
//#include "GeneralExtensionFilter.h"
//#include "StandardUVCFilter.h"
#include "DeviceFactory.h"



#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <asm/types.h>
#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

#include <sstream>
#include <utility>
#include <thread>
#include <cstring>
#include <type_traits>

using namespace TopGear;
using namespace Linux;


//template <>
//std::vector<IGenericVCDeviceRef> DeviceFactory<DiscernibleVCDevice>::EnumerateDevices()
//{
//	std::vector<IGenericVCDeviceRef> result;
//	auto genericDevices = DeviceFactory<GenericVCDevice>::EnumerateDevices();
//	for (auto device : genericDevices)
//	{
//		auto gDevice = std::dynamic_pointer_cast<IMSource>(device);
//		if (gDevice == nullptr)
//			continue;
//		std::shared_ptr<IMExtensionLite> validator = std::make_shared<GeneralExtensionFilter>(gDevice->GetSource());
//		if (validator->IsValid())
//			result.push_back(std::make_shared<DiscernibleVCDevice>(device, validator));
//	}
//	return result;
//}
