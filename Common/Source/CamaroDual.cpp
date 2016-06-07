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

int CamaroDual::GetShutter(uint32_t &val)
{
    if (!masterCC)
        return -1;
    return masterCC->GetShutter(val);
}

int CamaroDual::SetShutter(uint32_t val)
{
    if (masterCC && slaveCC)
        return masterCC->SetExposure(val) |
               slaveCC->SetExposure(val);
    return -1;
}

int CamaroDual::GetExposure(bool &ae, float &ev)
{
    if (!masterCC)
        return -1;
    return masterCC->GetExposure(ae, ev);
}

int CamaroDual::SetExposure(bool ae, float ev)
{
    if (masterCC && slaveCC)
        return masterCC->SetExposure(ae, ev) |
               slaveCC->SetExposure(ae, ev);
    return -1;
}

int CamaroDual::GetGain(float &gainR, float &gainG, float &gainB)
{
	if (!masterCC)
		return -1;
	return masterCC->GetGain(gainR, gainG, gainB);
}

int CamaroDual::SetGain(float gainR, float gainG, float gainB)
{
	if (masterCC && slaveCC)
		return masterCC->SetGain(gainR, gainG, gainB) |
			   slaveCC->SetGain(gainR, gainG, gainB);
	return -1;
}

bool CamaroDual::SetControl(std::string name, IPropertyData &val)
{
	return masterDC->SetControl(name, val) && slaveDC->SetControl(name, val);
}
bool CamaroDual::SetControl(std::string name, IPropertyData &&val)
{
	return masterDC->SetControl(name, val) && slaveDC->SetControl(name, val);
}
bool CamaroDual::GetControl(std::string name, IPropertyData &val)
{
	return masterDC->GetControl(name, val);
}

bool CamaroDual::StartStream()
{
	if (masterDC && slaveDC)
	{
        uint16_t num = RESYNC_NUM;
		masterDC->SetControl("Trigger", PropertyData<uint8_t>(0));
        masterDC->SetControl("Resync", PropertyData<uint16_t>(num));
        slaveDC->SetControl("Resync", PropertyData<uint16_t>(num));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		auto camera0 = std::dynamic_pointer_cast<CameraSoloBase>(videoStreams[0]);
		auto camera1 = std::dynamic_pointer_cast<CameraSoloBase>(videoStreams[1]);
		if (camera0 == nullptr || camera1 == nullptr)
			return false;
		camera0->CameraSoloBase::StartStream();
		camera1->CameraSoloBase::StartStream();
		while (!camera0->IsStreaming() || !camera1->IsStreaming())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		frameWatchThread = std::thread(&CamaroDual::FrameWatcher, this);
		threadOn = frameWatchThread.joinable();
		masterDC->SetControl("Trigger", PropertyData<uint8_t>(1));
		return true;
	}
	return false;
}

bool CamaroDual::StopStream()
{
	auto result = videoStreams[1]->StopStream() && videoStreams[0]->StopStream();
	if (frameWatchThread.joinable())
	{
		frameBuffer.Discard();
		frameWatchThread.join();
	}
	threadOn = false;
	return result;
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

int CamaroDual::SetRegisters(uint16_t regaddr[], uint16_t regval[], int num)
{
	auto ll1 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[0]);
	auto ll2 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[1]);
	if (ll1 == nullptr || ll2 == nullptr)
		return -1;
	return ll1->SetRegisters(regaddr, regval, num) | ll2->SetRegisters(regaddr, regval, num);
}

int CamaroDual::GetRegisters(uint16_t regaddr[], uint16_t regval[], int num)
{
	auto ll1 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[0]);
	auto ll2 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[1]);
	if (ll1 == nullptr || ll2 == nullptr)
		return -1;
	return ll1->GetRegisters(regaddr, regval, num) | ll2->GetRegisters(regaddr, regval, num);
}

int CamaroDual::SetRegister(uint16_t regaddr, uint16_t regval)
{
	auto ll1 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[0]);
	auto ll2 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[1]);
	if (ll1 == nullptr || ll2 == nullptr)
		return -1;
	return ll1->SetRegister(regaddr, regval) | ll2->SetRegister(regaddr, regval);
}

int CamaroDual::GetRegister(uint16_t regaddr, uint16_t& regval)
{
	auto ll1 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[0]);
	auto ll2 = std::dynamic_pointer_cast<ILowlevelControl>(videoStreams[1]);
	if (ll1 == nullptr || ll2 == nullptr)
		return -1;
	return ll1->GetRegister(regaddr, regval) | ll2->GetRegister(regaddr, regval);
}

void CamaroDual::FrameWatcher()
{
	//RESYNC_NUM
    uint64_t lastIndex = UINT64_MAX;
	auto dropped = 0;
	std::vector<IVideoFramePtr> frameVector[2];
	std::pair<int, IVideoFramePtr> frameEx;
	while (frameBuffer.Pop(frameEx))
	{
        frameVector[frameEx.first].emplace_back(std::move(frameEx.second));

		auto found = false;
		for (auto mit = frameVector[0].begin(); mit != frameVector[0].end(); ++mit)
		{
			auto &mFrame = *mit;
			for (auto sit = frameVector[1].begin(); sit != frameVector[1].end(); ++sit)
			{
				auto &sFrame = *sit;
                if (mFrame->GetFrameIndex() == sFrame->GetFrameIndex())
				{
                    if (lastIndex == UINT64_MAX)
                        lastIndex = mFrame->GetFrameIndex();
					else
					{
						if (++lastIndex >= RESYNC_NUM)
							lastIndex = 0;
                        if (mFrame->GetFrameIndex() >= lastIndex)
                            dropped = mFrame->GetFrameIndex() - lastIndex;
						else
                            dropped = RESYNC_NUM - lastIndex + mFrame->GetFrameIndex();
						if (dropped > 0)
						{
                            lastIndex = mFrame->GetFrameIndex();
                            //std::cout << dropped << " Frames dropped before Index " << lastIndex << std::endl;
						}
					}
					//std::cout << " Frame " << mFrame->GetFrameIdx() << std::endl;
                    auto vector = std::shared_ptr<std::vector<IVideoFramePtr>>(new std::vector<IVideoFramePtr> { mFrame, sFrame } );
                    Notify(vector);
					if (fnCb)
                        fnCb(*this, *vector);
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
