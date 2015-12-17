#pragma once

#include "IVideoFrame.h"

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

		virtual void GetSize(int &width, int &height) override
		{
			width = actualWidth;
			height = actualHeight;
		}

		explicit VideoFrameEx(std::shared_ptr<IVideoFrame> &vf,
			uint32_t headOffset, uint32_t stride, uint32_t width, uint32_t height, uint16_t index,
			uint32_t extraOffset, uint32_t extraSize
			)
			: frame(vf), offset(headOffset), actualStride(stride),
			  actualWidth(width), actualHeight(height), idx(index),
			  extra(extraOffset), extraLen(extraSize)
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
		uint32_t extra;
		uint32_t extraLen;
	};
}
