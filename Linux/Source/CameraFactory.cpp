#include "CameraFactory.h"

using namespace TopGear;
using namespace Linux;


std::shared_ptr<IVideoStream> CreateCamaroDual(std::vector<IGenericVCDeviceRef> &list)
{
    std::shared_ptr<IVideoStream> master;
    std::shared_ptr<IVideoStream> slave;
    for(auto item : list)
    {
        auto camera = std::dynamic_pointer_cast<Camaro>(CameraFactory<Camaro>::CreateInstance(item));
        if (camera->QueryDeviceRole() == 0 && master == nullptr)
        {
            master = std::static_pointer_cast<IVideoStream>(camera);
        }
        if (camera->QueryDeviceRole() == 1 && slave == nullptr)
        {
            slave = std::static_pointer_cast<IVideoStream>(camera);
        }
        if (master!=nullptr && slave!=nullptr)
            return std::make_shared<CamaroDual>(master, slave);
    }
    return {};
}
