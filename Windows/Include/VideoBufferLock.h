#pragma once

#include "System.h"
#include "IVideoFrame.h"

namespace TopGear
{
	namespace Win
	{
		//-------------------------------------------------------------------
		//  VideoBufferLock class
		//
		//  Locks a video buffer that might or might not support IMF2DBuffer.
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
				return actualStride*height;
			}

			virtual void GetSize(int &w, int &h) override
			{
				w = width;
				h = height;
			}

			explicit VideoBufferLock(IMFMediaBuffer *pBuffer,
				LONGLONG timestamp, //In 100-nanosecond
				LONG lDefaultStride, DWORD dwWidthInPixels, DWORD dwHeightInPixels)
				: m_p2DBuffer(nullptr),
				  m_bLocked(false),
				  defaultStride(lDefaultStride),
				  actualStride(0),
				  width(dwWidthInPixels),
				  height(dwHeightInPixels)
			{
				m_pBuffer = pBuffer;
				m_pBuffer->AddRef();

				tm.tv_sec = long(timestamp / 10000000);
				tm.tv_usec = long(timestamp / 10 - tm.tv_sec * 1000000);

				// Query for the 2-D buffer interface. OK if this fails.
				(void)m_pBuffer->QueryInterface(IID_PPV_ARGS(&m_p2DBuffer));
			}

			virtual ~VideoBufferLock()
			{
				VideoBufferLock::UnlockBuffer();
				System::SafeRelease(&m_pBuffer);
				System::SafeRelease(&m_p2DBuffer);
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
				uint32_t *pStride,        // Receives the actual stride.
				uint8_t** ppExtra = nullptr
				) override
			{
				HRESULT hr;
				// Use the 2-D version if available.
				if (m_p2DBuffer)
				{
					hr = m_p2DBuffer->Lock2D(ppScanLine0, &actualStride);
					*pStride = actualStride;
				}
				else
				{
					// Use non-2D version.
					BYTE *pData = nullptr;

					hr = m_pBuffer->Lock(&pData, nullptr, nullptr);
					if (SUCCEEDED(hr))
					{
						*pStride = actualStride = defaultStride;
						*ppScanLine0 = pData;
					}
				}

				m_bLocked = (SUCCEEDED(hr));

				return hr;
			}

			//-------------------------------------------------------------------
			// UnlockBuffer
			//
			// Unlocks the buffer. Called automatically by the destructor.
			//-------------------------------------------------------------------

			virtual void UnlockBuffer() override
			{
				if (m_bLocked)
				{
					if (m_p2DBuffer)
					{
						(void)m_p2DBuffer->Unlock2D();
					}
					else
					{
						(void)m_pBuffer->Unlock();
					}
					m_bLocked = false;
				}
			}

		private:
			IMFMediaBuffer  *m_pBuffer;
			IMF2DBuffer     *m_p2DBuffer;
			bool            m_bLocked;
			long defaultStride;
			long actualStride;
			long width;
			long height;
			timeval tm;
		};
	}
}