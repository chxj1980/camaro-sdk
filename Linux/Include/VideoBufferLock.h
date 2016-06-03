#pragma once

#include "IVideoFrame.h"

namespace TopGear
{
    namespace Linux
	{
		//-------------------------------------------------------------------
		//  VideoBufferLock class
		//
		//
		//-------------------------------------------------------------------

		class VideoBufferLock : public IVideoFrame
		{
		public:
			virtual uint32_t GetExtraLength() const override { return 0; }
            virtual uint64_t GetFrameIndex() const override { return frameIndex; }
            virtual uint64_t GetTimestamp() const override { return tm; }

			virtual uint32_t GetLength() const override
			{
				if (!m_bLocked)
					return 0;
                if (frameSize)
                    return frameSize;
                return defaultStride*videoFormat.Height;
			}

            virtual const VideoFormat &GetFormat() const override
            {
                return videoFormat;
            }

            explicit VideoBufferLock(int dev, int index, unsigned char *pdata,
                uint64_t timestamp, uint64_t frameNo,
                int lDefaultStride, const VideoFormat &format, uint32_t size=0)
                : handle(dev),
                  bufferIndex(index),
                  pBuffer(pdata),
				  m_bLocked(false),
                  defaultStride(lDefaultStride),
                  videoFormat(format),
                  tm(timestamp),
                  frameIndex(frameNo),
                  frameSize(size)
            {
			}

			virtual ~VideoBufferLock()
			{
			}

			//-------------------------------------------------------------------
			// LockBuffer
			//
			// Locks the buffer. Returns a pointer to scan line 0 and returns the stride.
			//
			// The caller must provide the default stride as an input parameter, in case
			// the buffer does not expose IMF2DBuffer. You can calculate the default stride
			// from the media type.
			//-------------------------------------------------------------------

			virtual int LockBuffer(
				uint8_t **ppScanLine0,    // Receives a pointer to the start of scan line 0.
                uint32_t *pStride,          // Receives the actual stride.
				uint8_t **ppExtra = nullptr
				) override
			{
                (void)ppExtra;
                *ppScanLine0 = pBuffer;
                *pStride = defaultStride;
                m_bLocked = true;
                return 0;
			}

			//-------------------------------------------------------------------
			// UnlockBuffer
			//
			// Unlocks the buffer. Called automatically by the destructor.
			//-------------------------------------------------------------------

			virtual void UnlockBuffer() override
			{
                m_bLocked = false;
			}

		private:
            int handle;
            int bufferIndex;
            unsigned char *pBuffer;
            bool m_bLocked;
			long defaultStride;
            const VideoFormat videoFormat;
            uint64_t tm;
            uint64_t frameIndex;
            uint32_t frameSize;
		};
	}
}
