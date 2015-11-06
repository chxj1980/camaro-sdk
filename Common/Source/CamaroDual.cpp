#include "CamaroDual.h"
#include <thread>
#include <iostream>
#include <CameraBase.h>

using namespace TopGear;

CamaroDual::CamaroDual(std::shared_ptr<IVideoStream>& master, std::shared_ptr<IVideoStream> &slave)
{
	videoStreams.push_back(master);
	videoStreams.push_back(slave);
	masterCC = std::dynamic_pointer_cast<TopGear::ICameraControl>(master);
	slaveCC = std::dynamic_pointer_cast<TopGear::ICameraControl>(slave);
	masterDC = std::dynamic_pointer_cast<IDeviceControl>(master);
	slaveDC = std::dynamic_pointer_cast<IDeviceControl>(slave);
	master->RegisterFrameCallback(std::bind(&CamaroDual::OnMasterFrame, this, std::placeholders::_1));
	slave->RegisterFrameCallback(std::bind(&CamaroDual::OnSlaveFrame, this, std::placeholders::_1));
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

bool CamaroDual::StartStream(int formatIndex)
{
	if (masterDC && slaveDC && formatIndex>=0)
	{
		masterDC->SetSensorTrigger(0);
		masterDC->SetResyncNumber(RESYNC_NUM);
		slaveDC->SetResyncNumber(RESYNC_NUM);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		auto camera0 = std::dynamic_pointer_cast<CameraBase>(videoStreams[0]);
		auto camera1 = std::dynamic_pointer_cast<CameraBase>(videoStreams[1]);
		if (camera0 == nullptr || camera1 == nullptr)
			return false;
		camera0->CameraBase::StartStream(formatIndex);
		camera1->CameraBase::StartStream(formatIndex);
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

void CamaroDual::FrameWatcher()
{
	//RESYNC_NUM
	int lastIndex = -1;
	auto dropped = 0;
	std::vector<IVideoFrameRef> frameVector[2];
	std::pair<int, IVideoFrameRef> frameEx;
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
					auto vector = std::vector<IVideoFrameRef>{ mFrame, sFrame };
					if (fnCb)
						fnCb(vector);
					if (pCbobj)		//deprecated
						pCbobj->OnFrame(vector);
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

		//if (framePair[one])		//Frame drop!
		//{
		//	droppedIndex = framePair[one]->GetFrameIdx(); //Log dropped frame index
		//	std::cout<<"Device "<< other << " Frame "<<
		//		framePair[one]->GetFrameIdx()<<" Dropped"<<std::endl; 
		//}
		//framePair[one] = frame.second;

		//if (framePair[0] && framePair[1])
		//{
		//	
		//	if (framePair[0]->GetFrameIdx() != framePair[1]->GetFrameIdx()) //Frame mismatch!
		//	{
		//		/*std::cout << "Device " << other << " Frame " <<
		//			framePair[other]->GetFrameIdx() << " Mismatch" << std::endl;*/
		//		std::cout << "Mismatch" << std::endl;
		//		std::cout << " Device 0" << " Frame " << framePair[0]->GetFrameIdx() << std::endl;
		//		std::cout << " Device 1" << " Frame " << framePair[1]->GetFrameIdx() << std::endl;
		//		//Discard older one
		//		if (abs(framePair[0]->GetFrameIdx() - framePair[1]->GetFrameIdx()) == 1)
		//		{
		//			if (framePair[0]->GetFrameIdx() < framePair[1]->GetFrameIdx())
		//				framePair[0].reset();
		//			else
		//				framePair[1].reset();
		//			
		//		}
		//		else
		//		{
		//			if (framePair[0]->GetFrameIdx() < framePair[1]->GetFrameIdx())
		//				framePair[1].reset();
		//			else
		//				framePair[0].reset();
		//		}
		//		continue;
		//	}
		//	droppedIndex = -1; //reset state
		//	auto vector = std::vector<IVideoFrameRef> {framePair[0], framePair[1]};
		//	if (fnCb)
		//		fnCb(vector);
		//	if (pCbobj)		//deprecated
		//		pCbobj->OnFrame(vector);
		//	framePair[0].reset();
		//	framePair[1].reset();
		//}
	//}
}

void CamaroDual::OnMasterFrame(std::vector<IVideoFrameRef>& frames)
{
	if (frames.size() != 1)
		return;
	if (threadOn)
		frameBuffer.Push(make_pair(0, frames[0]));
}

void CamaroDual::OnSlaveFrame(std::vector<IVideoFrameRef>& frames)
{
	if (frames.size() != 1)
		return;
	if (threadOn)
		frameBuffer.Push(make_pair(1, frames[0]));
}
