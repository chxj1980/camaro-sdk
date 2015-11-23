#include "ExtensionAccess.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

using namespace TopGear;
using namespace Linux;

ExtensionAccess::ExtensionAccess(int dev, std::shared_ptr<ExtensionFilterBase> &validator)
    : handle(dev), extensionAgent(validator)
{
    unitId = extensionAgent->GetExtensionInfo()->UnitId;
}

std::unique_ptr<uint8_t[]> ExtensionAccess::GetProperty(int index, int& len)
{
	len = extensionAgent->GetLen(index);
    std::unique_ptr<uint8_t[]> data(new uint8_t[len]);
    std::memset(data.get(),0,len);
    uvc_xu_control_query qry;
    qry.unit = unitId;//XU unit id
    qry.selector = index;
    qry.size = len;
    qry.query = UVC_GET_CUR;
    qry.data = data.get();
    if (ioctl(handle,UVCIOC_CTRL_QUERY, &qry)==-1)
        return {};
	return data;
}

int ExtensionAccess::SetProperty(int index, const uint8_t* data, size_t size)
{
    uvc_xu_control_query qry;
    qry.unit = unitId;//XU unit id
    qry.selector = index;
    qry.size = size;
    qry.query = UVC_SET_CUR;
    qry.data = const_cast<uint8_t *>(data);
    return ioctl(handle,UVCIOC_CTRL_QUERY, &qry);
}

ExtensionAccess::~ExtensionAccess()
{
}
