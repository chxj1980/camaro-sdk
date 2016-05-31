#pragma once

#include "IGenericVCDevice.h"
#include "VideoFormat.h"
#include "FileSource.h"

namespace TopGear
{
    class ImageDevice : public IGenericVCDevice
    {
    public:
        virtual std::string GetSymbolicLink() override { return "Virtual Link";}
        virtual std::string GetFriendlyName() override { return "Image Device";}
        virtual std::string GetDeviceInfo() override { return "";}
        virtual std::shared_ptr<ISource> &GetSource() override
        {
            return source;
        }
        const VideoFormat &GetFormat() const { return format; }

        ImageDevice(std::shared_ptr<FileSource> &s, const VideoFormat &f)
            : source(s), format(f)
        {

        }

        ImageDevice(std::shared_ptr<FileSource> &s, VideoFormat &&f)
            : source(s), format(f)
        {

        }

        virtual ~ImageDevice() {}

    private:
        std::shared_ptr<ISource> source;
        const VideoFormat format;
    };
}

