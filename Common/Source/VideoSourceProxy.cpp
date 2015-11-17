#include "VideoSourceProxy.h"
#include <thread>
#include <chrono>

using namespace TopGear;

VideoSourceProxy::VideoSourceProxy(std::shared_ptr<IMultiVideoSource>& reader, uint32_t index)
	: client(reader), streamIndex(index), formats(reader->GetAllFormats(index))
{
	client->RegisterReaderCallback(streamIndex, std::bind(&VideoSourceProxy::OnFrame, this, std::placeholders::_1));
}

const VideoFormat& VideoSourceProxy::GetCurrentFormat() const
{
	if (currentFormatIndex < 0)
		return VideoFormatNull;
	return formats[currentFormatIndex];
}

bool VideoSourceProxy::StartStream()
{
	if (currentFormatIndex < 0)
		return false;
	if (!client->StartStream(streamIndex))
		return false;
	while (!client->IsStreaming(streamIndex))
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return true;
}

bool VideoSourceProxy::StopStream()
{
	return client->StopStream(streamIndex);
}

bool VideoSourceProxy::IsStreaming() const
{
	return client->IsStreaming(streamIndex);
}

void VideoSourceProxy::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fncb = fn;
}

void VideoSourceProxy::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	fncb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

void VideoSourceProxy::OnFrame(IVideoFramePtr& frame)
{
	if (fncb == nullptr)
		return;
	std::vector<IVideoFramePtr> v = { frame };
	fncb(*this, v);
}

int VideoSourceProxy::GetMatchedFormatIndex(const VideoFormat& format) const
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

bool VideoSourceProxy::SetCurrentFormat(uint32_t formatIndex)
{
	if (!client->SetCurrentFormat(streamIndex, formatIndex))
		return false;
	currentFormatIndex = formatIndex;
	return true;
}


int VideoSourceProxy::GetOptimizedFormatIndex(VideoFormat& format, const char *fourcc)
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