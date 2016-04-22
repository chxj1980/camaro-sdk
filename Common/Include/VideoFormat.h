#pragma once

namespace TopGear
{
	struct VideoFormat
	{
		int Width;
		int Height;
		int MaxRate;
		char PixelFormat[4];	//FourCC

        static const VideoFormat Null;
	};

    //VideoFormat VideoFormat::Null;
}
