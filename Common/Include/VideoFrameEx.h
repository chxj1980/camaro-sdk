#pragma once

#include "IVideoFrame.h"
#include <cstring>

namespace TopGear
{
	class VideoFrameEx final
		: public IVideoFrame
	{
	public:
		virtual uint32_t GetExtraLength() const override
		{
			return extraLen;
		}

		virtual int LockBuffer(uint8_t** ppData, uint32_t* pStride, uint8_t** ppExtra = nullptr) override
		{
			uint8_t *pBuffer;
			auto result = frame->LockBuffer(&pBuffer, pStride);
			if (actualStride>0)
				*pStride = actualStride;
			*ppData = pBuffer + offset;
			if (ppExtra)
			{
				*ppExtra = extraLen > 0 ? pBuffer + extra : nullptr;
			}
			return result;
		}
		virtual void UnlockBuffer() override
		{
			frame->UnlockBuffer();
		}
        virtual uint64_t GetFrameIndex() const override
		{
			return idx;
		}
        virtual uint64_t GetTimestamp() const override
		{
			return timestamp;//frame->GetTimestamp();
		}
		virtual uint32_t GetLength() const override
		{
			return actualStride*actualHeight;
		}

        virtual const VideoFormat &GetFormat() const override
		{
			return format;
		}

		explicit VideoFrameEx(std::shared_ptr<IVideoFrame> &vf,
			uint32_t headOffset, uint32_t stride, uint32_t width, uint32_t height, uint16_t index,
			uint32_t extraOffset, uint32_t extraSize, uint32_t cc=0, uint64_t tm=0) :
			  frame(vf), offset(headOffset), actualStride(stride),
			  actualWidth(width), actualHeight(height), idx(index),
			  extra(extraOffset), extraLen(extraSize), fourcc(cc),
			  timestamp(tm==0?vf->GetTimestamp():tm)
		{
            format = frame->GetFormat();
            format.Width = actualWidth;
            format.Height = actualHeight;
            if (fourcc > 0)
                std::memcpy(format.PixelFormat, &fourcc, 4);
		}

		virtual ~VideoFrameEx()
		{
		}
	protected:
		std::shared_ptr<IVideoFrame> frame;
        VideoFormat format;
		const uint32_t offset;
		const uint32_t actualStride;
		const uint32_t actualWidth;
		const uint32_t actualHeight;
		const uint16_t idx;
		const uint32_t extra;
		const uint32_t extraLen;
		const uint32_t fourcc;
		const uint64_t timestamp;
	};
}
