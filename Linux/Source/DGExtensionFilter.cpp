#include "DGExtensionFilter.h"
#include "ExtensionRepository.h"
#include <iostream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>
#include <linux/media.h>
#include "v4l2helper.h"


using namespace TopGear;
using namespace Linux;

DGExtensionFilter::DGExtensionFilter(std::shared_ptr<IGenericVCDevice> &device)
    : ExtensionFilterBase(device, ExtensionRepository::DGXuCode)
{

}

std::string DGExtensionFilter::GetDeviceInfo()
{
    if (pInfo==nullptr || isValid==false)
        return {};
    if (deviceInfo.empty())
    {
        std::unique_ptr<uint8_t[]> data(new uint8_t[controlLens[DeviceInfoCode]]);
        std::memset(data.get(),0,controlLens[DeviceInfoCode]);
        uvc_xu_control_query qry;
        qry.unit = pInfo->UnitId;//XU unit id
        qry.selector = DeviceInfoCode;
        qry.size = controlLens[DeviceInfoCode];
        qry.query = UVC_GET_CUR;
        qry.data = data.get();
        if (ioctl(handle, UVCIOC_CTRL_QUERY, &qry)==-1)
        {
            //printf("Unable to get property value\n");
            deviceInfo.clear();
        }
        else
            deviceInfo = std::string(reinterpret_cast<char *>(data.get()));
    }
    return deviceInfo;
}
