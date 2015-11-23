#include "GenericVCDevice.h"
#include "LSource.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;

std::string GenericVCDevice::GetFriendlyName()
{
    return name;
}

std::string GenericVCDevice::GetSymbolicLink()
{
    return symbolicLink;
}

GenericVCDevice::GenericVCDevice(std::shared_ptr<ISource> &vsource)
    : source(vsource)
{
    auto ls = std::dynamic_pointer_cast<LSource>(source);
    if (ls)
    {
        name = std::string(reinterpret_cast<const char *>(ls->GetCapability().card));
        symbolicLink = ls->GetName();
    }
}

GenericVCDevice::~GenericVCDevice()
{
    auto ls = std::dynamic_pointer_cast<LSource>(source);
    if (ls)
        ::close(ls->GetHandle());
}
