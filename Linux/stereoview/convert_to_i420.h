/*
 * ConvertYUYVToI420.h
 *
 *  Created on: May 10, 2016
 *      Author: nick
 */

#pragma once

#include "IProcessor.h"
#include "ProcessorBase.h"
#include "VideoFormat.h"
#include "buffer_manager.h"

using namespace TopGear;

//namespace LibraF
//{


class ConvertToI420
        : public ProcessorBase<std::vector<IVideoFramePtr>>,
          public IProcessorResult<std::vector<IVideoFramePtr>>
{
public:
	typedef std::shared_ptr<BufferManager<IVideoFrame, 2, 50>> FrameManagerPtr;
	ConvertToI420(FrameManagerPtr &manager);
	virtual ~ConvertToI420();
    virtual bool ProcessImp(std::vector<IVideoFramePtr> &source) override;
    virtual std::shared_ptr<std::vector<IVideoFramePtr>> GetResult();
protected:
	FrameManagerPtr frameManager;
	std::vector<IVideoFramePtr> frames;
	void ColorConvert(uint8_t *source, uint8_t *i420, const VideoFormat &format);
};

//} /* namespace LibraF */
