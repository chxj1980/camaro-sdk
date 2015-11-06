#pragma once
#include "IVideoStream.h"
#include "CameraBase.h"

namespace TopGear
{
	class StandardUVC :public CameraBase
	{
	public:
		virtual const VideoFormat& GetCurrentFormat() const override
		{
			return pReader->GetCurrentFormat();
		}

		virtual int GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc="") override
		{
			return pReader->GetOptimizedFormatIndex(format, fourcc);
		}
		virtual int GetMatchedFormatIndex(const VideoFormat& format) const override
		{
			return pReader->GetMatchedFormatIndex(format);
		}
		virtual const std::vector<VideoFormat>& GetAllFormats() const override
		{
			return pReader->GetAllFormats();
		}

		explicit StandardUVC(std::shared_ptr<IVideoStream> &vs)
			: CameraBase(vs) {}
		virtual ~StandardUVC() {}
	};
}

