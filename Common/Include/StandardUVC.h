#pragma once
#include "IVideoStream.h"
#include "CameraBase.h"

namespace TopGear
{
	class StandardUVC :public CameraBase
	{
	public:
		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc="") override
		{
			return pReader->GetOptimizedFormatIndex(format, fourcc);
		}
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override
		{
			return GetMatchedFormatIndex(format);
		}
		virtual const std::vector<VideoFormat>& GetAllFormats() const override
		{
			return GetAllFormats();
		}

		explicit StandardUVC(std::shared_ptr<IVideoStream> &vs)
			: CameraBase(vs) {}
		virtual ~StandardUVC() {}
	};
}

