#pragma once
#include <string>
#include <functional>

#include <mfidl.h>
#include <Mfreadwrite.h>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"


namespace TopGear
{
	namespace Win
	{
		const UINT WM_APP_PREVIEW_ERROR = WM_APP + 1;    // wparam = HRESULT

		class VideoSourceReader : public IMFSourceReaderCallback, public IVideoStream
		{
		public:
			
			static HRESULT CreateInstance(
				HWND hVideo,
				HWND hEvent,
				IMFMediaSource *pSource,
				VideoSourceReader **ppPlayer
				);

			// IUnknown methods
			STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override;
			STDMETHODIMP_(ULONG) AddRef() override;
			STDMETHODIMP_(ULONG) Release() override;

			// IMFSourceReaderCallback methods
			STDMETHODIMP OnReadSample(
				HRESULT hrStatus,
				DWORD dwStreamIndex,
				DWORD dwStreamFlags,
				LONGLONG llTimestamp,
				IMFSample *pSample
				) override;

			STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override;
			STDMETHODIMP OnFlush(DWORD) override;

			HRESULT CloseDevice();
			//HRESULT       ResizeVideo(WORD width, WORD height);
			//HRESULT       CheckDeviceLost(DEV_BROADCAST_HDR *pHdr, BOOL *pbDeviceLost);

			virtual int GetOptimizedFormatIndex(VideoFormat &format, const char *fourcc = "") override;
			virtual int GetMatchedFormatIndex(const VideoFormat &format) const override;

			virtual const std::vector<VideoFormat> &GetAllFormats() const override
			{ return videoFormats; }

			virtual const VideoFormat& GetCurrentFormat() const override
			{
				return videoFormats[currentFormatIndex];
			}
			
			virtual bool StartStream(int formatIndex) override;
			virtual bool StopStream() override;
			virtual bool IsStreaming() const override
			{ return streamOn; }
			virtual void RegisterFrameCallback(IVideoFrameCallback *pCB) override;
			virtual void RegisterFrameCallback(const VideoFrameCallbackFn& fn) override;
		private:
			int currentFormatIndex = 0;
		protected:
			VideoFrameCallbackFn fnCb = nullptr;
			IVideoFrameCallback *pCbobj = nullptr;
			std::vector<VideoFormat> videoFormats;

			bool IsFormatSupported(const GUID &subtype) const;
			HRESULT OpenMediaSource(IMFMediaSource *pSource);
			void EnumerateFormats();
			// Constructor is private. Use static CreateInstance method to create.
			VideoSourceReader(HWND hVideo, HWND hEvent);

			// Destructor is private. Caller should call Release.
			virtual ~VideoSourceReader();

			void NotifyError(HRESULT hr) const
			{
				PostMessage(m_hwndEvent, WM_APP_PREVIEW_ERROR, static_cast<WPARAM>(hr), 0L);
			}

			long                    m_nRefCount;        // Reference count.
			CRITICAL_SECTION        m_critsec;

			HWND                    m_hwndVideo;        // Video window.
			HWND                    m_hwndEvent;        // Application window to receive events. 

			IMFSourceReader         *m_pReader;

			bool isRunning = false;
			bool streamOn = false;
			long defaultStride = 0;
			int frameWidth = 0;
			int frameHeight = 0;
			
		};
	}
}