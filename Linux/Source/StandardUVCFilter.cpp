#include "StandardUVCFilter.h"
#include "GeneralExtensionFilter.h"
#include "v4l2helper.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;

StandardUVCFilter::StandardUVCFilter(int dev)
    :isValid(false)
{
    v4l2_capability cap;
    ioctl(dev,VIDIOC_QUERYCAP,&cap);
    auto xu = v4l2Helper::GetXUFromBusInfo(cap);
    if (xu)
    {
        GeneralExtensionFilter filter(dev,xu);
        isValid = !filter.IsValid();
    }
    else
        isValid = true;
}

StandardUVCFilter::~StandardUVCFilter()
{
}
