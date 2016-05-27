#include "FlyCaptureDevice.h"
#include "FlyCaptureSource.h"

using namespace TopGear;

std::shared_ptr<FlyCapture2::BusManager> BusMgr;

void FlyCaptureDevice::EnumVideoDeviceSources(std::vector<IGenericVCDevicePtr> &inventory,
                                              std::vector<uint32_t> sns)
{
    if (BusMgr==nullptr)
        BusMgr.reset(new FlyCapture2::BusManager);

    if (sns.size()==0)
    {
        uint32_t numCameras;

        auto error = BusMgr->GetNumOfCameras(&numCameras);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            //PrintError(error);
            //return SENSOR_NUMBER_ERROR;
            return;
        }
        for(auto i=0u; i<numCameras; ++i)
        {
            auto device = std::make_shared<FlyCaptureDevice>(i, false);
            if (device->IsValid())
                inventory.emplace_back(std::static_pointer_cast<IGenericVCDevice>(device));
        }
    }
    else
    {
        for(auto sn : sns)
        {
            auto device = std::make_shared<FlyCaptureDevice>(sn, true);
            if (device->IsValid())
                inventory.emplace_back(std::static_pointer_cast<IGenericVCDevice>(device));
        }
    }
    BusMgr.reset();
}

FlyCaptureDevice::FlyCaptureDevice(uint32_t token, bool isSN)
{
    if (BusMgr==nullptr)
        BusMgr.reset(new FlyCapture2::BusManager);

    FlyCapture2::Error error;

    //Get GUID token of camera
    FlyCapture2::PGRGuid guid;
    if (isSN)
        error = BusMgr->GetCameraFromSerialNumber(token, &guid);
    else
        error = BusMgr->GetCameraFromIndex(token, &guid);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        valid = false;
        return;
    }
    //Check USB 3.0 interface
    FlyCapture2::InterfaceType iftype;
    error = BusMgr->GetInterfaceTypeFromGuid(&guid, &iftype);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //PrintError(error);
        valid = false;
        return;
    }
    if (iftype != FlyCapture2::INTERFACE_USB3)
    {
        //LOG(ERROR)<< "Sensor Interface Type is not USB 3.0!" << endl;
        valid = false;
        return;
    }

    auto fc = std::make_shared<FlyCaptureSource>(guid);
    source = std::static_pointer_cast<ISource>(fc);
    valid = fc->IsAvailable();

    FlyCapture2::CameraInfo info;
    if (fc->GetCameraInfo(info))
    {
        driverName = std::string(info.driverName);
        modelName = std::string(info.modelName);
        sensorInfo = std::to_string(info.serialNumber);
    }
    BusMgr.reset();
}
