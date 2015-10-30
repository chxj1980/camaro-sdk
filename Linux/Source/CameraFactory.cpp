#include "CameraFactory.h"
#include "StandardUVC.h"
#include "Camaro.h"
//#include "GenericVCDevice.h"
//#include "ExtensionAccess.h"
//#include "System.h"
//#include "VideoSourceReader.h"
#include "CamaroDual.h"

using namespace TopGear;
using namespace Linux;


template <class T>
std::shared_ptr<IVideoStream> CameraComboFactory<T>::CreateInstance(std::vector<IGenericVCDeviceRef> &devices)
{
	return{};
}
