#pragma once

#include "IVideoFrame.h"

namespace TopGear
{
	class VideoFrameEx final
		: public IVideoFrame
	{
	public:
		virtual int LockBuffer(uint8_t** pData, uint32_t* pStride) override
		{
			uint8_t *pBuffer;
			auto result = frame->LockBuffer(&pBuffer, pStride);
			*pData = pBuffer + offset;
			return result;
		}
		virtual void UnlockBuffer() override
		{
			frame->UnlockBuffer();
		}
		virtual uint16_t GetFrameIdx() const override
		{
			return idx;
		}
		virtual timeval GetTimestamp() const override
		{
			return frame->GetTimestamp();
		}
		virtual uint32_t GetLength() const override
		{
			return actualStride*actualHeight;
		}

		explicit VideoFrameEx(std::shared_ptr<IVideoFrame> &vf,
			uint32_t headOffset, uint32_t stride, uint32_t width, uint32_t height, uint16_t index
			)
			: frame(vf), offset(headOffset), actualStride(stride),
			  actualWidth(width), actualHeight(height), idx(index)
		{
		}

		virtual ~VideoFrameEx()
		{
		}
	protected:
		std::shared_ptr<IVideoFrame> frame;
		uint32_t offset;
		uint32_t actualStride;
		uint32_t actualWidth;
		uint32_t actualHeight;
		uint16_t idx;
	};
}
