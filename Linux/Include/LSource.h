#pragma once
#include <string>
#include "IGenericVCDevice.h"

#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

namespace TopGear
{
    namespace Linux
	{
        typedef std::pair<std::string, int> SourcePair;

        class LSource : public ISource
		{
		public:
            explicit LSource(const SourcePair &pair)
                : handle(pair.second), name(pair.first)
            {
                ioctl(handle, VIDIOC_QUERYCAP, &cap);
            }
            virtual ~LSource() = default;
            int GetHandle() const { return handle; }
            std::string GetName() const { return name; }
            const v4l2_capability &GetCapability() const {return cap;}
        private:
            int handle;
            std::string name;
            v4l2_capability cap;
		};
	}
}
