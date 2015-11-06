#include "VideoSourceReader.h"

#include <shlwapi.h>
#include <mfapi.h>
#include <string>
#include <sstream>
#include "MFHelper.h"
#include "System.h"
#include <iostream>
#include <functional>
#include "VideoBufferLock.h"
#include <thread>

using namespace TopGear;
using namespace Win;
//-------------------------------------------------------------------
//  CreateInstance
//
//  Static class method to create the CPreview object.
//-------------------------------------------------------------------

HRESULT VideoSourceReader::CreateInstance(
	HWND hVideo,        // Handle to the video window.
	HWND hEvent,        // Handle to the window to receive notifications.
	IMFMediaSource *pSource,
	VideoSourceReader **ppPlayer // Receives a pointer to the VideoSourceReader object.
	)
{
	//assert(hVideo != NULL);
	//assert(hEvent != NULL);

	if (ppPlayer == nullptr)
	{
		return E_POINTER;
	}

	auto pPlayer = new (std::nothrow) VideoSourceReader(hVideo, hEvent);

	// The CPlayer constructor sets the ref count to 1.

	if (pPlayer == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	//pPlayer->Initialize();

	auto hr = pPlayer->OpenMediaSource(pSource);

	if (SUCCEEDED(hr))
	{
		*ppPlayer = pPlayer;
		(*ppPlayer)->AddRef();
		pPlayer->EnumerateFormats();
	}

	System::SafeRelease(&pPlayer);
	return hr;
}


//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

VideoSourceReader::VideoSourceReader(HWND hVideo, HWND hEvent) :
	m_nRefCount(1),
	m_hwndVideo(hVideo),
	m_hwndEvent(hEvent),
	m_pReader(nullptr)
{
	InitializeCriticalSection(&m_critsec);
}

//-------------------------------------------------------------------
//  destructor
//-------------------------------------------------------------------

VideoSourceReader::~VideoSourceReader()
{
	CloseDevice();

	//m_draw.DestroyDevice();

	DeleteCriticalSection(&m_critsec);
}


//-------------------------------------------------------------------
//  Initialize
//
//  Initializes the object.
//-------------------------------------------------------------------

//HRESULT VideoSourceReader::Initialize()
//{
//	auto hr = S_OK;

//	//hr = m_draw.CreateDevice(m_hwndVideo);

//	return hr;
//}


//-------------------------------------------------------------------
//  CloseDevice
//
//  Releases all resources held by this object.
//-------------------------------------------------------------------

HRESULT VideoSourceReader::CloseDevice()
{
	EnterCriticalSection(&m_critsec);

	System::SafeRelease(&m_pReader);

	LeaveCriticalSection(&m_critsec);
	return S_OK;
}

HRESULT VideoSourceReader::OpenMediaSource(IMFMediaSource* pSource)
{
	IMFAttributes   *pAttributes = nullptr;

	EnterCriticalSection(&m_critsec);

	// Release the current device, if any.
	auto hr = CloseDevice();

	//
	// Create the source reader.
	//

	// Create an attribute store to hold initialization settings.

	if (SUCCEEDED(hr))
	{
		hr = MFCreateAttributes(&pAttributes, 2);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
	}

	// Set the callback pointer.
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUnknown(
			MF_SOURCE_READER_ASYNC_CALLBACK,
			this
			);
	}

	if (SUCCEEDED(hr))
	{
		hr = MFCreateSourceReaderFromMediaSource(
			pSource,
			pAttributes,
			&m_pReader
			);
	}

	if (FAILED(hr))
	{
		if (pSource)
		{
			pSource->Shutdown();

			// NOTE: The source reader shuts down the media source
			// by default, but we might not have gotten that far.
		}
		CloseDevice();
	}

	System::SafeRelease(&pAttributes);

	LeaveCriticalSection(&m_critsec);
	return hr;
}

void VideoSourceReader::EnumerateFormats()
{
	videoFormats.clear();
	if (m_pReader == nullptr)
		return;

	EnterCriticalSection(&m_critsec);

	int rate, den, width, height;
	for (auto i = 0;; i++)
	{
		IMFMediaType *pType;
		auto hr = m_pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType);
		if (FAILED(hr))
		{
			System::SafeRelease(&pType);
			break;
		}

		GUID subtype = { 0 };
		hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
		if (FAILED(hr))
		{
			System::SafeRelease(&pType);
			break;
		}

		MFHelper::GetAttributeFrameRate(pType, rate, den);
		rate /= den;
		MFHelper::GetAttributeSize(pType, width, height);

		VideoFormat format = { width,height,rate };
		memcpy(format.PixelFormat, &subtype.Data1, 4);
		videoFormats.emplace_back(format);
		System::SafeRelease(&pType);
	}

	LeaveCriticalSection(&m_critsec);
}

int VideoSourceReader::GetMatchedFormatIndex(const VideoFormat& format) const
{
	if (m_pReader == nullptr)
		return -1;
	auto index = -1;
	for (auto i : videoFormats)
	{
		index++;
		if (format.Width > 0 && format.Width != i.Width)
			continue;
		if (format.Height> 0 && format.Height != i.Height)
			continue;
		if (format.MaxRate > 0 && format.MaxRate != i.MaxRate)
			continue;
		if (strcmp(format.PixelFormat, "")!=0 && strncmp(format.PixelFormat, i.PixelFormat, 4)!=0)
			continue;
		return index;
	}
	return -1;
}


int VideoSourceReader::GetOptimizedFormatIndex(VideoFormat& format, const char *fourcc)
{
	if (m_pReader == nullptr)
		return -1;

	auto wCurrent = 0, hCurrent = 0, rCurrent = 0;
	auto index = -1;
	auto i = -1;
	for(auto f : videoFormats)
	{
		++i;
		if (strcmp(fourcc, "") != 0 && strncmp(fourcc, f.PixelFormat, 4) != 0)
			continue;
		if (f.Height >= hCurrent && f.MaxRate >= rCurrent)
		{
			wCurrent = f.Width;
			hCurrent = f.Height;
			rCurrent = f.MaxRate;
			index = i;
		}
	}
	if (index >= 0)
		format = videoFormats[index];
	return index;
}

bool VideoSourceReader::StartStream(int formatIndex)
{
	StopStream();

	EnterCriticalSection(&m_critsec);

	IMFMediaType *pType;
	auto hr = m_pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, formatIndex, &pType);
	if (SUCCEEDED(hr))
	{
		currentFormatIndex = formatIndex;
		isRunning = true;
		hr = m_pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
		MFHelper::GetAttributeSize(pType, frameWidth, frameHeight);
		MFHelper::GetDefaultStride(pType, defaultStride);
		//hr = m_pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, true);

		////Ask for the first sample.
		hr = m_pReader->ReadSample(
			static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
			0,
			nullptr,
			nullptr,
			nullptr,
			nullptr
			);
	}
	System::SafeRelease(&pType);
	LeaveCriticalSection(&m_critsec);
	return hr==S_OK;
}

bool VideoSourceReader::StopStream()
{
	isRunning = false;
	while (streamOn)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return true;
}

/////////////// IUnknown methods ///////////////

//-------------------------------------------------------------------
//  AddRef
//-------------------------------------------------------------------

ULONG VideoSourceReader::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}


//-------------------------------------------------------------------
//  Release
//-------------------------------------------------------------------

ULONG VideoSourceReader::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return uCount;
}



//-------------------------------------------------------------------
//  QueryInterface
//-------------------------------------------------------------------

HRESULT VideoSourceReader::QueryInterface(REFIID riid, void** ppv)
{
#pragma warning( push )
#pragma warning( disable : 4838 )
	static const QITAB qit[] =
	{
		QITABENT(VideoSourceReader, IMFSourceReaderCallback),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
#pragma warning( pop )
}


/////////////// IMFSourceReaderCallback methods ///////////////

//-------------------------------------------------------------------
// OnReadSample
//
// Called when the IMFMediaSource::ReadSample method completes.
//-------------------------------------------------------------------

HRESULT VideoSourceReader::OnReadSample(
	HRESULT hrStatus,
	DWORD index,
	DWORD flags,
	LONGLONG timeStamp/* llTimestamp */,
	IMFSample *pSample      // Can be NULL
	)
{
	
	auto hr = S_OK;
	IMFMediaBuffer *pBuffer = nullptr;

	EnterCriticalSection(&m_critsec);

	//if (FAILED(hrStatus))
	//{
	//	hr = hrStatus;
	//}

	//if (SUCCEEDED(hr))
	//{
	if (pSample)
	{
		OutputDebugStringW(L"Frame Arrival\n");
		// Get the video frame buffer from the sample.

		hr = pSample->GetBufferByIndex(0, &pBuffer);

		// Draw the frame.
		if (SUCCEEDED(hr) && (pCbobj != nullptr || fnCb !=nullptr))
		{
			std::vector<std::shared_ptr<IVideoFrame>> frames;
			auto vbl = std::static_pointer_cast<IVideoFrame>(
				std::make_shared<VideoBufferLock>(
					pBuffer, timeStamp, defaultStride, frameWidth, frameHeight));
			frames.push_back(vbl);
			//Invoke callback handler
			if (pCbobj)
				pCbobj->OnFrame(*this, frames);
			else
				fnCb(*this, frames);
		}
	}
	//}

	// Request the next frame.
	if (SUCCEEDED(hr) && isRunning)
	{
		hr = m_pReader->ReadSample(
			static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
			0,
			nullptr,   // actual
			nullptr,   // flags
			nullptr,   // timestamp
			nullptr    // sample
			);
		streamOn = true;
	}
	else
	{
		isRunning = false;
		streamOn = false;
	}

	if (FAILED(hr))
	{
		NotifyError(hr);
	}
	System::SafeRelease(&pBuffer);

	LeaveCriticalSection(&m_critsec);
	return hr;
}

STDMETHODIMP VideoSourceReader::OnEvent(DWORD, IMFMediaEvent *)
{
	return S_OK;
}

STDMETHODIMP VideoSourceReader::OnFlush(DWORD)
{
	return S_OK;
}


//-------------------------------------------------------------------
// TryMediaType
//
// Test a proposed video format.
//-------------------------------------------------------------------

void VideoSourceReader::RegisterFrameCallback(IVideoFrameCallback* pCB)
{
	pCbobj = pCB;
}

void VideoSourceReader::RegisterFrameCallback(const VideoFrameCallbackFn& fn)
{
	fnCb = fn;
}

bool VideoSourceReader::IsFormatSupported(const GUID &subtype) const
{
	//Do we support this type directly ?
	return true;
}


//-------------------------------------------------------------------
//  CheckDeviceLost
//  Checks whether the current device has been lost.
//
//  The application should call this method in response to a
//  WM_DEVICECHANGE message. (The application must register for 
//  device notification to receive this message.)
//-------------------------------------------------------------------

/*HRESULT CPreview::CheckDeviceLost(DEV_BROADCAST_HDR *pHdr, BOOL *pbDeviceLost)
{
	DEV_BROADCAST_DEVICEINTERFACE *pDi = NULL;

	if (pbDeviceLost == NULL)
	{
		return E_POINTER;
	}

	*pbDeviceLost = FALSE;

	if (pHdr == NULL)
	{
		return S_OK;
	}

	if (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
	{
		return S_OK;
	}

	pDi = (DEV_BROADCAST_DEVICEINTERFACE*)pHdr;


	EnterCriticalSection(&m_critsec);

	if (m_pwszSymbolicLink)
	{
		if (_wcsicmp(m_pwszSymbolicLink, pDi->dbcc_name) == 0)
		{
			*pbDeviceLost = TRUE;
		}
	}

	LeaveCriticalSection(&m_critsec);

	return S_OK;
}*/