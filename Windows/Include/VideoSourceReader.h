#pragma once
#include <string>
#include <functional>
#include <map>

#include <shlwapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "VideoFormat.h"
#include "IMultiVideoSource.h"
#include <mutex>
#include <condition_variable>

namespace TopGear
{
	namespace Win
	{
		const UINT WM_APP_PREVIEW_ERROR = WM_APP + 1;    // wparam = HRESULT

		class VideoSourceReader : 
			public IMFSourceReaderCallback,
			public IMultiVideoSource
		{
		public:
			static std::vector<std::shared_ptr<IVideoStream>> CreateVideoStreams(std::shared_ptr<IGenericVCDevice> &pSource);
			static std::shared_ptr<IVideoStream> CreateVideoStream(std::shared_ptr<IGenericVCDevice> &pSource);
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

			virtual const std::vector<VideoFormat> &GetAllFormats(uint32_t index) override;
			virtual bool SetCurrentFormat(uint32_t index, int formatIndex) override;
			virtual bool StartStream(uint32_t index) override;
			virtual bool StopStream(uint32_t index) override;
			virtual bool IsStreaming(uint32_t index) override;
			virtual void RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn) override;

			virtual ~VideoSourceReader();
		protected:
			struct StreamState
			{
				std::vector<VideoFormat> formats;
				ReaderCallbackFn fncb = nullptr;
				long defaultStride = 0;
				int frameWidth = 0;
				int frameHeight = 0;
				bool isRunning = false;
				bool streamOn = false;
			};

			std::map<uint32_t, StreamState> streams;

			//bool IsFormatSupported(const GUID &subtype) const;
			HRESULT OpenMediaSource(IMFMediaSource *pSource);

			void EnumerateStreams(bool onlyFirst = false);
			void EnumerateFormats(uint32_t index, std::vector<VideoFormat> &videoFormats) const;
			// Constructor is private. Use static CreateInstance method to create.
			VideoSourceReader();
			

			void NotifyError(HRESULT hr) const
			{
				//PostMessage(m_hwndEvent, WM_APP_PREVIEW_ERROR, static_cast<WPARAM>(hr), 0L);
			}

			long                    m_nRefCount;        // Reference count.
			std::mutex mtx;
			std::condition_variable cv;
			//CRITICAL_SECTION        m_critsec;

			//HWND                    m_hwndVideo;        // Video window.
			//HWND                    m_hwndEvent;        // Application window to receive events. 

			IMFSourceReader         *m_pReader;
		};
	}
}
