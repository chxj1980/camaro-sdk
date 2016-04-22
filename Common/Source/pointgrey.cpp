#include "pointgrey.h"
#include <flycapture/FlyCapture2.h>

using namespace TopGear;

PointGrey::PointGrey(std::shared_ptr<IVideoStream> &vs)
    : CameraSoloBase(vs) {}
