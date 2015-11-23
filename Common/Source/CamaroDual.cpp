#include "CamaroDual.h"
#include <thread>
#include <iostream>
#include "CameraSoloBase.h"

using namespace TopGear;

CamaroDual::CamaroDual(std::shared_ptr<IVideoStream>& master, std::shared_ptr<IVideoStream> &slave)
{
	videoStreams.push_back(master);
	videoStreams.push_back(slave);
	masterCC = std::dynamic_pointer_cast<TopGear::ICameraControl>(master);
	slaveCC = std::dynamic_pointer_cast<TopGear::ICameraControl>(slave);
	masterDC = std::dynamic_pointer_cast<IDeviceControl>(master);
	slaveDC = std::dynamic_pointer_cast<IDeviceControl>(slave);
	master->RegisterFrameCallback(std::bind(&CamaroDual::OnMasterFrame, this, std::placeholders::_1, std::placeholders::_2));
	slave->RegisterFrameCallback(std::bind(&CamaroDual::OnSlaveFrame, this, std::placeholders::_1, std::placeholders::_2));
}

CamaroDual::~CamaroDual()
{
	CamaroDual::StopStream();
}

int CamaroDual::Flip(bool vertical, bool horizontal)
{
	if (masterCC && slaveCC)
		return masterCC->Flip(vertical, horizontal) | 
			   slaveCC->Flip(vertical, horizontal);
	return -1;
}

int CamaroDual::GetExposure(uint16_t& val)
{
	if (!masterCC)
		return -1;
	return masterCC->GetExposure(val);
}

int CamaroDual::SetExposure(uint16_t val)
{
	if (masterCC && slaveCC)
		return masterCC->SetExposure(val) |
			   slaveCC->SetExposure(val);
	return -1;
}

int CamaroDual::GetGain(uint16_t& gainR, uint16_t& gainG, uint16_t& gainB)
{
	if (!masterCC)
		return -1;
	return masterCC->GetGain(gainR, gainG, gainB);
}

int CamaroDual::SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB)
{
	if (masterCC && slaveCC)
		return masterCC->SetGain(gainR, gainG, gainB) |
			   slaveCC->SetGain(gainR, gainG, gainB);
	return -1;
}

int CamaroDual::SetSensorTrigger(uint8_t level)
{
	if (!masterDC)
		return -1;
	return masterDC->SetSensorTrigger(level);
}

int CamaroDual::SetResyncNumber(uint16_t resyncNum)
{
	if (!masterDC)
		return -1;
	return masterDC->SetResyncNumber(resyncNum);
}


int CamaroDual::QueryDeviceRole()
{
	return -1;
}

std::string CamaroDual::QueryDeviceInfo()
{
	if (masterDC && slaveDC)
		return masterDC->QueryDeviceInfo() + "+" + slaveDC->QueryDeviceInfo();
	return "";
}

bool CamaroDual::StartStream()
{
	if (masterDC && slaveDC)
	{
		masterDC->SetSensorTrigger(0);
		masterDC->SetResyncNumber(RESYNC_NUM);
		slaveDC->SetResyncNumber(RESYNC_NUM);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		auto camera0 = std::dynamic_pointer_cast<CameraSoloBase>(videoStreams[0]);
		auto camera1 = std::dynamic_pointer_cast<CameraSoloBase>(videoStreams[1]);
		if (camera0 == nullptr || camera1 == nullptr)
			return false;
		camera0->CameraSoloBase::StartStream();
		camera1->CameraSoloBase::StartStream();
		while (!camera0->IsStreaming() || !camera1->IsStreaming())
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		frameWatchThread = std::thread(&CamaroDual::FrameWatcher, this);
		threadOn = frameWatchThread.joinable();
		masterDC->SetSensorTrigger(1);
		return true;
	}
	return false;
}

bool CamaroDual::StopStream()
{
	videoStreams[1]->StopStream();
	videoStreams[0]->StopStream();
	if (frameWatchThread.joinable())
	{
		frameBuffer.Discard();
		frameWatchThread.join();
	}
	threadOn = false;
	return true;
}

bool CamaroDual::IsStreaming() const
{
	return videoStreams[1]->IsStreaming() && videoStreams[0]->IsStreaming();
}

int CamaroDual::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
	return videoStreams[0]->GetOptimizedFormatIndex(format, fourcc);
}

int CamaroDual::GetMatchedFormatIndex(const VideoFormat& format) const
{
	return videoStreams[0]->GetMatchedFormatIndex(format);
}

const std::vector<VideoFormat>& CamaroDual::GetAllFormats() const
{
	return videoStreams[0]->GetAllFormats();
}

const VideoFormat& CamaroDual::GetCurrentFormat() const
{
	return videoStreams[0]->GetCurrentFormat();
}

bool CamaroDual::SetCurrentFormat(uint32_t formatIndex)
{
	return videoStreams[0]->SetCurrentFormat(formatIndex) &&
		   videoStreams[1]->SetCurrentFormat(formatIndex);
}

void CamaroDual::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fnCb = fn;
}

void CamaroDual::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	fnCb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

void CamaroDual::Notify(std::vector<IVideoFramePtr>& payload)
{
	if (fnCb)
		fnCb(*this, payload);
}

void CamaroDual::Register(std::shared_ptr<IProcessor>& p)
{
	processor = p;
}

void CamaroDual::FrameWatcher()
{
	//RESYNC_NUM
	auto lastIndex = -1;
	auto dropped = 0;
	std::vector<IVideoFramePtr> frameVector[2];
	std::pair<int, IVideoFramePtr> frameEx;
	while (true)
	{
		if (!frameBuffer.Pop(frameEx))
			break;
		//if (frame.second->GetFrameIdx() == droppedIndex)
		//	continue;  //Discard lagged frame

		auto one = frameEx.first;
		//auto other = one ? 0 : 1;

		frameVector[one].push_back(frameEx.second);

		auto found = false;
		for (auto mit = frameVector[0].begin(); mit != frameVector[0].end(); ++mit)
		{
			auto &mFrame = *mit;
			for (auto sit = frameVector[1].begin(); sit != frameVector[1].end(); ++sit)
			{
				auto &sFrame = *sit;
				if (mFrame->GetFrameIdx() == sFrame->GetFrameIdx())
				{
					if (lastIndex == -1)
						lastIndex = mFrame->GetFrameIdx();
					else
					{
						if (++lastIndex >= RESYNC_NUM)
							lastIndex = 0;
						if (mFrame->GetFrameIdx() >= lastIndex)
							dropped = mFrame->GetFrameIdx() - lastIndex;
						else
							dropped = RESYNC_NUM - lastIndex + mFrame->GetFrameIdx();
						if (dropped > 0)
						{
							lastIndex = mFrame->GetFrameIdx();
							std::cout << dropped << " Frames dropped before Index " << lastIndex << std::endl;
						}
					}
					//std::cout << " Frame " << mFrame->GetFrameIdx() << std::endl;
					auto vector = std::vector<IVideoFramePtr>{ mFrame, sFrame };
					if (fnCb)
					{
						if (processor)
							processor->Process(*this, vector);
						else
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

void CamaroDual::OnMasterFrame(IVideoStream &master, std::vector<IVideoFramePtr>& frames)
{
    (void)master;
	if (frames.size() != 1)
		return;
	if (threadOn)
		frameBuffer.Push(std::make_pair(0, frames[0]));
}

void CamaroDual::OnSlaveFrame(IVideoStream &slave, std::vector<IVideoFramePtr>& frames)
{
    (void)slave;
	if (frames.size() != 1)
		return;
	if (threadOn)
		frameBuffer.Push(std::make_pair(1, frames[0]));
}
