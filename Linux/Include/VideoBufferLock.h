#pragma once

#include "IVideoFrame.h"
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>

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
			virtual uint16_t GetFrameIdx() const override { return 0; }
			virtual timeval GetTimestamp() const override { return tm; }

			virtual uint32_t GetLength() const override
			{
				if (!m_bLocked)
					return 0;
                return defaultStride*height;
			}

            explicit VideoBufferLock(int dev, int index, unsigned char *pdata,
                timeval timestamp, //In 100-nanosecond
                int lDefaultStride, int dwWidthInPixels, int dwHeightInPixels)
                : handle(dev),
                  bufferIndex(index),
                  pBuffer(pdata),
				  m_bLocked(false),
				  defaultStride(lDefaultStride),
				  width(dwWidthInPixels),
                  height(dwHeightInPixels),
                  tm(timestamp)
            {
			}

			virtual ~VideoBufferLock()
			{
                if (pBuffer)
                {
                    v4l2_buffer queue_buf;
                    std::memset(&queue_buf,0,sizeof(queue_buf));
                    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    queue_buf.memory = V4L2_MEMORY_MMAP;
                    queue_buf.index = bufferIndex;
                    if (ioctl(handle, VIDIOC_QBUF, &queue_buf)==0)
                        pBuffer = nullptr;
                }
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
				uint32_t *pStride          // Receives the actual stride.
				uint8_t **ppExtra = nullptr
				) override
			{
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
			long width;
			long height;
			timeval tm;
		};
	}
}
