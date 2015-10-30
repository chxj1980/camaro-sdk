#include "GenericVCDevice.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;

std::string GenericVCDevice::GetFriendlyName()
{
    if (name.empty())
    {
        v4l2_capability cap;
        auto hr = ioctl(handle,VIDIOC_QUERYCAP,&cap);
        if (hr==0)
            name = std::string(reinterpret_cast<char *>(cap.card));
    }
    return name;
}

GenericVCDevice::GenericVCDevice(std::string dev, int fd)
    : symbol(dev), handle(fd)
{
}

GenericVCDevice::~GenericVCDevice()
{
    if (handle>-1)
        ::close(handle);
}
