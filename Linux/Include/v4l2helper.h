#ifndef V4L2HELPER_H
#define V4L2HELPER_H

#include <vector>
#include <chrono>
#include <memory>
#include <LSource.h>
#include <ExtensionInfo.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

namespace TopGear
{
    namespace Linux
    {
        class v4l2Helper
        {
        public:
            static void EnumVideoDeviceSources(std::vector<SourcePair> &inventory,
                    std::chrono::milliseconds waitTime = std::chrono::milliseconds(0));
            static std::shared_ptr<ExtensionInfo> GetXUFromBusInfo(
                    const v4l2_capability &cap);
        };
    }
}

#endif // V4L2HELPER_H

