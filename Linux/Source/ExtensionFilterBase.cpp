#include "ExtensionFilterBase.h"
#include "LSource.h"
#include "v4l2helper.h"

using namespace TopGear;
using namespace Linux;

ExtensionFilterBase::ExtensionFilterBase(std::shared_ptr<IGenericVCDevice> &device, const std::array<uint8_t, 16> &xucode)
{
    auto source = std::dynamic_pointer_cast<LSource>(device->GetSource());
    if (source == nullptr)
        return;
    handle = source->GetHandle();
    pInfo = v4l2Helper::GetXUFromBusInfo(source->GetCapability());
    if (pInfo)
    {
        if (std::memcmp(pInfo->ExtensionCode,xucode.data(),16)==0)
        {
            isValid = true;
            ObtainInfo();
        }
    }
}

ExtensionFilterBase::~ExtensionFilterBase()
{
}

uint32_t ExtensionFilterBase::GetLen(int index, bool live)
{
    if (index < 1 || index>31)
        return 0;
    if (live)
    {
        uint16_t len;
        uvc_xu_control_query qry;
        qry.unit = pInfo->UnitId;//XU unit id
        qry.selector = index;
        qry.size = 2;
        qry.query = UVC_GET_LEN;
        qry.data = reinterpret_cast<uint8_t *>(&len);
        if (ioctl(handle,UVCIOC_CTRL_QUERY, &qry)==0)
            controlLens[index] = len;
        else
            return 0;
    }
    return controlLens[index];
}

bool ExtensionFilterBase::ObtainInfo()
{
    uint16_t len;
    uvc_xu_control_query qry;
    qry.unit = pInfo->UnitId;//XU unit id
    qry.selector = 0;
    qry.size = 2;
    qry.query = UVC_GET_LEN;
    qry.data = reinterpret_cast<uint8_t *>(&len);

    auto count = 0;
    for (auto i = 0; i < pInfo->ControlSize<<3; ++i)
    {
        if ((pInfo->Controls[i >> 3] & 1 << (i & 0x7)) == 0)
            continue;
        qry.selector = i+1;
        if (ioctl(handle, UVCIOC_CTRL_QUERY, &qry)==0)
            controlLens[i+1] = len;
        if (++count >= pInfo->NumControls)
            break;
    }
    return true;
}

