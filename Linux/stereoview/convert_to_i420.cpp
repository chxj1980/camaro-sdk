/*
 * ConvertYUYVToI420.cpp
 *
 *  Created on: May 10, 2016
 *      Author: nick
 */

#include "convert_to_i420.h"

#include "VideoBufferLock.h"
#include "libyuv.h"

#include <cstring>
#include <iostream>

using namespace TopGear::Linux;

//namespace LibraF
//{

ConvertToI420::ConvertToI420(FrameManagerPtr &manager)
	:frameManager(manager)
{
}

ConvertToI420::~ConvertToI420()
{
}

void ConvertToI420::ColorConvert(uint8_t *src, uint8_t *i420, const VideoFormat &format)
{
	uint8_t *dst_u = i420 + format.Width*format.Height;
    uint8_t *dst_v = dst_u+ (format.Width*format.Height)/4;
	int uvstride = format.Width>>1;

	if (std::memcmp(format.PixelFormat, "I420", 4)==0)
	{
		uint8_t *src_u = src + format.Width*format.Height;
        uint8_t *src_v = src_u+ (format.Width*format.Height)/4;
		libyuv::I420Copy(src, format.Width, src_u, uvstride, src_v, uvstride,
						 i420, format.Width, dst_u, uvstride, dst_v, uvstride,
						 format.Width, format.Height);
	}
	else if(std::memcmp(format.PixelFormat, "UYVY", 4)==0)
	{
		libyuv::UYVYToI420(src, format.Width<<1,
						   i420, format.Width, dst_u, uvstride, dst_v, uvstride,
						   format.Width, format.Height);
	}
	else //if (std::memcmp(format.PixelFormat, "YUY2", 4)==0 || std::memcmp(format.PixelFormat, "YUYV", 4)==0)
	{
		libyuv::YUY2ToI420(src, format.Width<<1,
						   i420, format.Width, dst_u, uvstride, dst_v, uvstride,
						   format.Width, format.Height);
	}

}

bool ConvertToI420::ProcessImp(std::vector<IVideoFramePtr> &source)
{
    static uint32_t count = 0;
    frames = frameManager->Update([&](std::vector<uint8_t *> &dst) {
		std::vector<std::shared_ptr<IVideoFrame>> result;
        for (auto i=0u; i<dst.size(); ++i)
		{
            if (i>=source.size() || source[i]==nullptr)
            {
                result.emplace_back(nullptr);
                continue;
            }
			//Get source data pointer
			uint8_t *pSrc = nullptr;
			uint32_t dump = 0;
            source[i]->LockBuffer(&pSrc, &dump);
            ColorConvert(pSrc, dst[i], source[i]->GetFormat());
            source[i]->UnlockBuffer();

            auto format = source[i]->GetFormat();
			std::memcpy(format.PixelFormat, "I420", 4);

			std::shared_ptr<IVideoFrame> frame =
                    std::make_shared<VideoBufferLock>(0, i, dst[i], source[i]->GetTimestamp(),
                                                      source[i]->GetFrameIndex(), format.Width, format,
													  format.Width*format.Height*3/2);
			result.emplace_back(std::move(frame));

		}
        std::cout<<"Converter Invoked! "<< count++ <<std::endl;
		return result;
	});
	return !frames.empty();
}

std::shared_ptr<std::vector<IVideoFramePtr>>  ConvertToI420::GetResult()
{

	if (frames.empty())
        return {};
    auto result = std::make_shared<std::vector<IVideoFramePtr>>();
    result->swap(frames);
    return result;
}

//} /* namespace LibraF */
