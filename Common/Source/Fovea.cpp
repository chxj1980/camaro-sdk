#include "Fovea.h"
#include "VideoFrameEx.h"

using namespace TopGear;

Fovea::Fovea(std::shared_ptr<IVideoStream> &wideangle, std::shared_ptr<IVideoStream> &telephoto)
{
    videoStreams.push_back(wideangle);
    videoStreams.push_back(telephoto);
    wideangle->RegisterFrameCallback(std::bind(&Fovea::OnWideAngleFrame, this, std::placeholders::_1, std::placeholders::_2));
    telephoto->RegisterFrameCallback(std::bind(&Fovea::OnTelephotoFrame, this, std::placeholders::_1, std::placeholders::_2));
}

Fovea::~Fovea()
{
    StopStreams();
}

void Fovea::FrameWatcher()
{
    IVideoFramePtr frames[2];
    std::pair<int, IVideoFramePtr> frameEx;
    while (frameBuffer.Pop(frameEx))
    {
        frames[frameEx.first] = std::move(frameEx.second);

        if (frames[0] && frames[1] && keyStreamIndex==frameEx.first)    //Key frame here and no empty frame
        {
            auto vector = std::shared_ptr<std::vector<IVideoFramePtr>>(new std::vector<IVideoFramePtr> { frames[0], frames[1] } );
            Notify(vector);
            if (fnCb)
                fnCb(*this, *vector);
        }
    }
}

void Fovea::PushFrame(int index, IVideoFramePtr &frame)
{
    if (threadOn)
    {
        frameBuffer.Push(std::make_pair(index, frame));
    }
}

void Fovea::OnWideAngleFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames)
{
    (void)source;
    if (frames.size() != 1)
        return;
    PushFrame(0, frames[0]);
}

void Fovea::OnTelephotoFrame(IVideoStream &source, std::vector<IVideoFramePtr> &frames)
{
    (void)source;
    if (frames.size() != 1)
        return;
    PushFrame(1, frames[0]);
}

const std::vector<std::shared_ptr<IVideoStream>> &Fovea::GetStreams() const
{
    return videoStreams;
}

bool Fovea::SelectStream(int index)
{
    if (index<0 || index>=int(videoStreams.size()))
        return false;
    currentStreamIndex = index;
    currentStream = videoStreams[index];
    return true;
}

bool Fovea::SelectStream(const std::shared_ptr<IVideoStream> &vs)
{
    auto i = 0;
    for(auto &item : videoStreams)
    {
        if (vs==item)
        {
            currentStreamIndex = i;
            currentStream = item;
            return true;
        }
        ++i;
    }
    return false;
}

int Fovea::GetCurrentStream(std::shared_ptr<IVideoStream> &current)
{
    if (videoStreams.empty() || currentStreamIndex>=int(videoStreams.size()))
        return -1;
    current = currentStream;
    return currentStreamIndex;
}

void Fovea::StartStreams()
{
    StopStreams();
    if (keyStreamIndex<0)
        return;
    for(auto &item : videoStreams)
    {
        if (item->GetCurrentFormat()==VideoFormat::Null)
        {
            StopStreams();
            return;
        }
        item->StartStream();
    }
    frameWatchThread = std::thread(&Fovea::FrameWatcher, this);
    threadOn = frameWatchThread.joinable();
}

void Fovea::StopStreams()
{
    for(auto &item : videoStreams)
        item->StopStream();
    if (frameWatchThread.joinable())
    {
        frameBuffer.Discard();
        frameWatchThread.join();
    }
    threadOn = false;
}

int Fovea::Flip(bool vertical, bool horizontal)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->Flip(vertical,horizontal);
}


int Fovea::GetShutter(uint32_t &val)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->GetShutter(val);
}

int Fovea::SetShutter(uint32_t val)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->SetShutter(val);
}

int Fovea::GetExposure(bool &ae, float &ev)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->GetExposure(ae, ev);
}

int Fovea::SetExposure(bool ae, float ev)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->SetExposure(ae, ev);
}

int Fovea::GetGain(float& gainR, float& gainG, float& gainB)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->GetGain(gainR, gainG, gainB);
}

int Fovea::SetGain(float gainR, float gainG, float gainB)
{
    auto cc = std::dynamic_pointer_cast<TopGear::ICameraControl>(currentStream);
    if (cc==nullptr)
        return -1;
    return cc->SetGain(gainR, gainG, gainB);
}

bool Fovea::StartStream()
{
    StartStreams();
    return threadOn;
}

bool Fovea::StopStream()
{
    StopStreams();
    return true;
}

bool Fovea::IsStreaming() const
{
    for(auto &item : videoStreams)
        if (item->IsStreaming())
            return true;
    return false;
}

bool Fovea::SetControl(std::string name, IPropertyData &val)
{
    auto dc = std::dynamic_pointer_cast<IDeviceControl>(currentStream);
    if (dc==nullptr)
        return -1;
    return dc->SetControl(name, val);
}

bool Fovea::SetControl(std::string name, IPropertyData &&val)
{
    auto dc = std::dynamic_pointer_cast<IDeviceControl>(currentStream);
    if (dc==nullptr)
        return -1;
    return dc->SetControl(name, val);
}

bool Fovea::GetControl(std::string name, IPropertyData &val)
{
    auto dc = std::dynamic_pointer_cast<IDeviceControl>(currentStream);
    if (dc==nullptr)
        return -1;
    return dc->GetControl(name, val);
}

int Fovea::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
    if (currentStream==nullptr)
        return -1;
    return currentStream->GetOptimizedFormatIndex(format, fourcc);
}

int Fovea::GetMatchedFormatIndex(const VideoFormat& format) const
{
    if (currentStream==nullptr)
        return -1;
    return currentStream->GetMatchedFormatIndex(format);
}

const std::vector<VideoFormat>& Fovea::GetAllFormats() const
{
//    if (currentStream==nullptr)
//        return -1;
    return currentStream->GetAllFormats();
}
const VideoFormat &Fovea::GetCurrentFormat() const
{
    if (currentStream==nullptr)
        return VideoFormat::Null;
    return currentStream->GetCurrentFormat();
}

bool Fovea::SetCurrentFormat(uint32_t formatIndex)
{
    if (currentStream==nullptr)
        return false;
    if  (!currentStream->SetCurrentFormat(formatIndex))
        return false;

    //Update keyStreamIndex with higher-rate stream
    int rate=0;
    int index=0;
    auto i = 0;
    for(auto &stream : videoStreams)
    {
        auto &format = stream->GetCurrentFormat();
        if (format!= VideoFormat::Null && format.MaxRate>rate)
        {
            rate = format.MaxRate;
            index = i;
        }
        ++i;
    }
    keyStreamIndex = index;
    return true;
}

void Fovea::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
    fnCb = fn;
}

void Fovea::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
    fnCb = std::bind(&IVideoFrameCallback::OnFrame, pCB, std::placeholders::_1, std::placeholders::_2);
}

