#ifndef _FLYCAPTURESOURCE_H
#define _FLYCAPTURESOURCE_H

#include "IGenericVCDevice.h"
#include <flycapture/FlyCapture2.h>


namespace TopGear
{
    class FlyCaptureSource : public ISource
    {
    public:
        FlyCaptureSource(FlyCapture2::PGRGuid &g)
        {
            cameraSource.Connect(&g);
            valid = cameraSource.IsConnected();
        }
        ~FlyCaptureSource()
        {
            if (cameraSource.IsConnected())
                cameraSource.Disconnect();
        }
        bool IsAvailable() const { return valid; }
        bool GetCameraInfo(FlyCapture2::CameraInfo &info)
        {
            return cameraSource.GetCameraInfo(&info)==FlyCapture2::PGRERROR_OK;
        }
        FlyCapture2::Camera &GetCameraSource() { return cameraSource; }
    private:
        FlyCapture2::Camera cameraSource;
        bool valid;
    };
}

#endif // _FLYCAPTURESOURCE_H

