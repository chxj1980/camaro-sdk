#pragma once

#include <cstring>
#include <string>

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

        VideoFormat(int w, int h, int rate, const std::string &fourcc = {})
            :Width(w), Height(h), MaxRate(rate)
        {
            if (fourcc.size()>=4)
                std::memcpy(PixelFormat, fourcc.c_str(), 4);
            else
                std::memset(PixelFormat, 0, 4);
        }

        ~VideoFormat() {}

        VideoFormat(const VideoFormat &rhs)
        {
            Width = rhs.Width;
            Height = rhs.Height;
            MaxRate = rhs.MaxRate;
            std::memcpy(PixelFormat, rhs.PixelFormat, 4);
        }

        friend bool operator ==(const VideoFormat& lhs, const VideoFormat& rhs)
        {
            return &lhs==&rhs;
        }

        friend bool operator !=(const VideoFormat& lhs, const VideoFormat& rhs)
        {
            return &lhs!=&rhs;
        }

        static const VideoFormat Null;
	};

    //VideoFormat VideoFormat::Null;
}
