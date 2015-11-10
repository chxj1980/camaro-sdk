#pragma once
#include <vector>
#include <functional>
#include "VideoFormat.h"
#include "IVideoFrame.h"

namespace TopGear
{
	class IMultiVideoSource
	{
	public:
		typedef std::function<void(IVideoFrameRef &)> ReaderCallbackFn;

		virtual ~IMultiVideoSource() = default;

		virtual const std::vector<VideoFormat> &GetAllFormats(uint32_t index) = 0;
		virtual bool SetCurrentFormat(uint32_t index, int formatIndex) = 0;
		virtual bool StartStream(uint32_t index) = 0;
		virtual bool StopStream(uint32_t index) = 0;
		virtual bool IsStreaming(uint32_t index) = 0;
		virtual void RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn) = 0;
	};
}
