#pragma once
#include "CameraSoloBase.h"

namespace TopGear
{
	class StandardUVC :public CameraSoloBase
	{
	public:
		virtual bool SetCurrentFormat(uint32_t formatIndex) override
		{
			return pReader->SetCurrentFormat(formatIndex);
		}

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
			: CameraSoloBase(vs) {}
		virtual ~StandardUVC() {}
	};
}

