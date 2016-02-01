#include "ImpalaE.h"
//#include <iostream>
#include <VideoFrameEx.h>

namespace TopGear
{
	ImpalaE::ImpalaE(std::vector<std::shared_ptr<IVideoStream>>& vs,std::shared_ptr<IExtensionAccess> &ex)
        : CameraBase(vs), xuAccess(ex), streaming(false)
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
            for (uint32_t index = 0; index < f.size(); ++index)
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
		uint32_t i = 0;
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

	bool ImpalaE::SetControl(std::string name, IPropertyData& val)
	{
        (void)name;
        (void)val;
		return false;
	}

	bool ImpalaE::SetControl(std::string name, IPropertyData&& val)
	{
        (void)name;
        (void)val;
		return false;
	}

	bool ImpalaE::GetControl(std::string name, IPropertyData& val)
	{
		if (name != "zdtable")
			return false;
		if (val.GetTypeHash() != typeid(std::vector<float>).hash_code())
			return false;

		auto len = 0;

		//Get handle
		auto data = xuAccess->GetProperty(0x0a, len);
		auto handle = data[0];

		xuAccess->SetProperty(0x0b, std::array<uint8_t, 0x10> {
			handle, 0x41, 0x05, 0x10, 0x00, 0x00, 0x00, 0x01,
			0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		data = xuAccess->GetProperty(0x0c, len, true);

		xuAccess->SetProperty(0x0b, std::array<uint8_t, 0x10> {
			handle, 0x41, 0x05, 0x10, 0x00, 0x00, 0x00, 0x02,
			0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		data = xuAccess->GetProperty(0x0c, len, true);

		//Get length of total data
		xuAccess->SetProperty(0x0b, std::array<uint8_t, 0x10> {
			handle, 0x41, 0x05, 0x10, 0x00, 0x00, 0x32, 0x03,
			0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		data = xuAccess->GetProperty(0x0c, len, true);
		if (len != 2)
		{
			xuAccess->SetProperty(0x0a, handle);
			return false;
		}
		uint32_t bytesLen = data[0] << 8 | data[1];
		std::unique_ptr<uint8_t[]> bytes(new uint8_t[bytesLen]);

		auto payload = std::array<uint8_t, 0x10> {
			handle, 0x41, 0x05, 0x00, 0x00, 0x32, 0x00, 0x00,
			0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		uint32_t offset = 0;
		//Read data by segment
		while (offset < bytesLen)
		{
			payload[6] = (offset >> 8) & 0xff;
			payload[7] = offset & 0xff;
			//Set offset
			xuAccess->SetProperty(0x0b, payload);
			//Read data with length reloaded
			data = xuAccess->GetProperty(0x0c, len, true);
			if (data==nullptr)	//Read failed
			{
				xuAccess->SetProperty(0x0a, handle);
				return false;
			}
			std::memcpy(bytes.get() + offset, data.get(), len);
			offset += len;
		}
		//Release handle
		xuAccess->SetProperty(0x0a, handle);

		//Convert bytes to float array
		auto &floatVal = *reinterpret_cast<PropertyData<std::vector<float>> *>(&val);
		for (uint32_t i = 0; i < bytesLen;i+=2)
		{
			if (i == 0)
				floatVal.Payload.push_back(0);
			else
				floatVal.Payload.push_back((bytes[i] << 8 | bytes[i + 1]) / 1000.0f);
		}

		return true;
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
        (void)fourcc;
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
						//std::stringstream ss;
						//ss << " Frame " << depthTime.tv_sec << "." << depthTime.tv_usec / 1000 << std::endl;
						//OutputDebugStringA(ss.str().c_str());
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
