#include "GeneralExtensionFilter.h"
#include <iostream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>
#include <linux/media.h>
#include <v4l2helper.h>


using namespace TopGear;
using namespace Linux;

const uint8_t GeneralExtensionFilter::guidCode[16]={
    0xff,0xff,0xff,0xff,
    0xff,0xff,
    0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

GeneralExtensionFilter::GeneralExtensionFilter(int dev)
    :handle(dev),unitId(-1)
{
    v4l2_capability cap;
    ioctl(handle,VIDIOC_QUERYCAP,&cap);
    pInfo = v4l2Helper::GetXUFromBusInfo(cap);
    if (pInfo)
    {
        if (std::memcmp(pInfo->ExtensionCode,guidCode,16)==0)
        {
            unitId = pInfo->UnitId;
            ObtainInfo();
        }
    }
}

GeneralExtensionFilter::GeneralExtensionFilter(int dev,std::shared_ptr<ExtensionInfo> &xu)
    :handle(dev),pInfo(xu)
{
    if (std::memcmp(pInfo->ExtensionCode,guidCode,16)==0)
    {
        unitId = pInfo->UnitId;
        ObtainInfo();
    }
}

GeneralExtensionFilter::~GeneralExtensionFilter()
{
}

std::string GeneralExtensionFilter::GetDeviceInfo()
{
	if (deviceInfo.empty())
	{
        std::unique_ptr<uint8_t[]> data(new uint8_t[controlLens[DeviceInfoCode]]);
        std::memset(data.get(),0,controlLens[DeviceInfoCode]);
        uvc_xu_control_query qry;
        qry.unit = unitId;//XU unit id
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

inline const std::shared_ptr<ExtensionInfo>& GeneralExtensionFilter::GetExtensionInfo() const
{
	return pInfo;
}

uint32_t GeneralExtensionFilter::GetLen(int index) const
{
	if (index < 1 || index>31)
		return 0;
	return controlLens[index];
}

bool GeneralExtensionFilter::ObtainInfo()
{
    uint16_t len;
    uvc_xu_control_query qry;
    for (auto i = 1; i <= pInfo->NumControls; ++i)
    {
        qry.unit = unitId;//XU unit id
        qry.selector = i;
        qry.size = 2;
        qry.query = UVC_GET_LEN;
        qry.data = reinterpret_cast<uint8_t *>(&len);
        if (ioctl(handle,UVCIOC_CTRL_QUERY, &qry)==0)
            controlLens[i] = len;
    }
    return true;
}
