#include "ImpalaE.h"
#include <iostream>
#include <VideoFrameEx.h>

namespace TopGear
{
	ImpalaE::ImpalaE(std::vector<std::shared_ptr<IVideoStream>>& vs,std::shared_ptr<IExtensionAccess> &ex)
		: CameraBase(vs), extensionAdapter(ex)
	{
		VideoFormat format;
		format.Width = 640;
		format.Height = 480;
		format.MaxRate = 30;
		std::memcpy(format.PixelFormat, "YUY2", 4);
		formats.push_back(format);

		for (auto &stream : videoStreams)
		{
			stream->RegisterFrameCallback(*stream, &ImpalaE::OnFrame, this);
			auto f = stream->GetAllFormats();
			for (auto index = 0; index < f.size(); ++index)
			{
				if (f[index].Height != 480 || f[index].Width == 640 || std::memcmp(f[index].PixelFormat, "YUY2", 4) != 0)
					continue;
				selectedFormats.emplace_back(index);
				break;
			}
		}

		ImpalaE::SetCurrentFormat(0);
	}

	void ImpalaE::OnFrame(IVideoStream& parent, std::vector<IVideoFramePtr>& frames)
	{
		if (!streaming)
			return;
		auto i = 0;
		for (i = 0; i < videoStreams.size();++i)
		{
			if (&parent == videoStreams[i].get())
				break;
		}
		if (i == videoStreams.size())
			return;
		//std::cout << i << std::endl;
		if (frameWatchThread.joinable())
			frameBuffer.Push(std::make_pair(i, std::move(frames[0])));
	}

	bool ImpalaE::StartStream()
	{
		for (auto &stream : videoStreams)
		{
			if (!stream->StartStream())
				return false;
		}
		while (!IsStreaming())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		streaming = true;
		frameWatchThread = std::thread(&ImpalaE::FrameWatcher, this);
		return true;
	}

	bool ImpalaE::StopStream()
	{
		streaming = false;
		auto result = true;
		for (auto &stream : videoStreams)
		{
			if (!stream->StopStream())
				result = false;
		}
		if (frameWatchThread.joinable())
		{
			frameBuffer.Discard();
			frameWatchThread.join();
		}
		return result;
	}

	bool ImpalaE::IsStreaming() const
	{
		for (auto &stream : videoStreams)
		{
			if (stream->IsStreaming())
				return true;
		}
		return false;
	}

	int ImpalaE::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
	{
		format = formats[0];
		return 0;
	}

	int ImpalaE::GetMatchedFormatIndex(const VideoFormat& format) const
	{
		(void)format;
		return 0;
	}

	const std::vector<VideoFormat>& ImpalaE::GetAllFormats() const
	{
		return formats;
	}

	const VideoFormat& ImpalaE::GetCurrentFormat() const
	{
		return formats[0];
	}

	bool ImpalaE::SetCurrentFormat(uint32_t formatIndex)
	{
		if (formatIndex != 0)
			return false;
		for (auto i = 0; i < videoStreams.size();++i)
			videoStreams[i]->SetCurrentFormat(selectedFormats[i]);
		return true;
	}

	void ImpalaE::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
	{
		fnCb = fn;
	}

	void ImpalaE::RegisterFrameCallback(IVideoFrameCallback* pCB)
	{
		fnCb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
	}

	void ImpalaE::FrameWatcher()
	{
		auto delta = 1000000 / GetCurrentFormat().MaxRate;

		static uint16_t index = 0;

		std::vector<IVideoFramePtr> frameVector[2];
		std::pair<int, IVideoFramePtr> frameEx;
		while (frameBuffer.Pop(frameEx))
		{
			//if (frameEx.second == nullptr)
			//	continue;
			frameVector[frameEx.first].emplace_back(std::move(frameEx.second));
			//frameEx.second.reset();

			auto found = false;
			for (auto mit = frameVector[0].begin(); mit != frameVector[0].end(); ++mit)
			{
				auto &rgbFrame = *mit;
				auto rgbTime = rgbFrame->GetTimestamp();
				for (auto sit = frameVector[1].begin(); sit != frameVector[1].end(); ++sit)
				{
					auto &depthFrame = *sit;
					auto depthTime = depthFrame->GetTimestamp();
					if (abs((rgbTime.tv_sec - depthTime.tv_sec)*1000000+(rgbTime.tv_usec - depthTime.tv_usec))<=delta)
					{
						std::stringstream ss;
						ss << " Frame " << depthTime.tv_sec << "." << depthTime.tv_usec / 1000 << std::endl;
						OutputDebugStringA(ss.str().c_str());
						auto rgbEx = std::make_shared<VideoFrameEx>(rgbFrame, 0, 0, rgbFrame->GetFormat().Width, rgbFrame->GetFormat().Height,
							index, 0, 0);
						auto depthEx = std::make_shared<VideoFrameEx>(depthFrame, 0, 0, depthFrame->GetFormat().Width * 2, depthFrame->GetFormat().Height,
							index, 0, 0, 0x59455247u); //FourCC "GREY"
						++index;
						auto vector = std::vector<IVideoFramePtr>{ rgbEx, depthEx };
						if (fnCb)
						{
							//if (processor)
							//	processor->Process(*this, vector);
							//else
							fnCb(*this, vector);
						}
						frameVector[1].erase(frameVector[1].begin(), sit + 1);
						frameVector[0].erase(frameVector[0].begin(), mit + 1);
						found = true;
						break;
					}
				}
				if (found)
					break;
			}
		}
	}
}
