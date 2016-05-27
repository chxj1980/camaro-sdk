#pragma once

#include <cstring>

namespace TopGear
{
	struct VideoFormat
	{
		int Width;
		int Height;
		int MaxRate;
		char PixelFormat[4];	//FourCC

        VideoFormat()
            :Width(0), Height(0), MaxRate(0)
        {
            std::memset(PixelFormat, 0, 4);
        }

        static const VideoFormat Null;
	};

    //VideoFormat VideoFormat::Null;
}
