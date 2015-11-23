#include "StandardUVCFilter.h"
#include "ExtensionRepository.h"
#include "LSource.h"

#include "v4l2helper.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;

StandardUVCFilter::StandardUVCFilter(std::shared_ptr<IGenericVCDevice> &device)
    :isValid(true)
{
    auto source = std::dynamic_pointer_cast<LSource>(device->GetSource());
    if (source == nullptr)
        return;
    auto pInfo = v4l2Helper::GetXUFromBusInfo(source->GetCapability());
    if (pInfo == nullptr)
        return;
    for (auto &xucode : ExtensionRepository::Inventory)
    {
        if (std::memcmp(pInfo->ExtensionCode,xucode.data(),16)==0)
        {
            isValid = true;
            break;
        }
    }
}
