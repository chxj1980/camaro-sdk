#include "DeviceFactory.h"

using namespace TopGear;
using namespace Win;

template<typename T>
const std::chrono::milliseconds DeviceFactory<T>::InitialTime = std::chrono::milliseconds(100);