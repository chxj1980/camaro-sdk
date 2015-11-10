#include "VideoSource.h"

using namespace TopGear;

VideoSource::VideoSource(std::shared_ptr<IMultiVideoSource>& reader, uint32_t index)
	: parent(reader), streamIndex(index), formats(reader->GetAllFormats(index))
{
	parent->RegisterReaderCallback(streamIndex, std::bind(&VideoSource::OnFrame, this, std::placeholders::_1));
}

bool VideoSource::StartStream(int formatIndex)
{
	if (!parent->SetCurrentFormat(streamIndex, formatIndex))
		return false;
	return parent->StartStream(streamIndex);
}

bool VideoSource::StopStream()
{
	return parent->StopStream(streamIndex);
}

bool VideoSource::IsStreaming() const
{
	return parent->IsStreaming(streamIndex);
}

void VideoSource::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fncb = fn;
}

void VideoSource::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	fncb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

void VideoSource::OnFrame(IVideoFrameRef& frame)
{
	if (fncb == nullptr)
		return;
	std::vector<IVideoFrameRef> v = { frame };
	fncb(*this, v);
}

int VideoSource::GetMatchedFormatIndex(const VideoFormat& format) const
{
	auto index = -1;
	for (auto i : formats)
	{
		index++;
		if (format.Width > 0 && format.Width != i.Width)
			continue;
		if (format.Height> 0 && format.Height != i.Height)
			continue;
		if (format.MaxRate > 0 && format.MaxRate != i.MaxRate)
			continue;
		if (strcmp(format.PixelFormat, "") != 0 && strncmp(format.PixelFormat, i.PixelFormat, 4) != 0)
			continue;
		return index;
	}
	return -1;
}


int VideoSource::GetOptimizedFormatIndex(VideoFormat& format, const char *fourcc)
{
	auto wCurrent = 0, hCurrent = 0, rCurrent = 0;
	auto index = -1;
	auto i = -1;
	for (auto f : formats)
	{
		++i;
		if (strcmp(fourcc, "") != 0 && strncmp(fourcc, f.PixelFormat, 4) != 0)
			continue;
		if (f.Height >= hCurrent && f.MaxRate >= rCurrent)
		{
			wCurrent = f.Width;
			hCurrent = f.Height;
			rCurrent = f.MaxRate;
			index = i;
		}
	}
	if (index >= 0)
		format = formats[index];
	return index;
}