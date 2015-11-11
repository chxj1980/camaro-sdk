#include "VideoSourceReader.h"

#include <string>
//#include <sstream>
#include "MFHelper.h"
#include "System.h"
//#include <iostream>
#include <thread>

#include "VideoBufferLock.h"
#include "VideoSource.h"

using namespace TopGear;
using namespace Win;

//-------------------------------------------------------------------
//  CreateInstance
//
//  Static class method to create the CPreview object.
//-------------------------------------------------------------------

std::vector<std::shared_ptr<IVideoStream>> VideoSourceReader::CreateInstances(std::shared_ptr<ISource> &pSource)
{
	std::shared_ptr<VideoSourceReader> pPlayer(new VideoSourceReader);
	if (pPlayer == nullptr)
		return{};// E_OUTOFMEMORY
	
	auto msource = std::dynamic_pointer_cast<IMSource>(pSource);
	if (msource==nullptr)
		return{};
	std::vector<std::shared_ptr<IVideoStream>> list;
	auto hr = pPlayer->OpenMediaSource(msource->GetSource());
	if (SUCCEEDED(hr))
	{
		pPlayer->EnumerateStreams();
		for (auto s : pPlayer->streams)
			list.emplace_back(std::make_shared<VideoSource>(
				std::static_pointer_cast<IMultiVideoSource>(pPlayer), s.first));
	}
	return list;
}


//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

VideoSourceReader::VideoSourceReader() :
	m_nRefCount(1),
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

const std::vector<VideoFormat>& VideoSourceReader::GetAllFormats(uint32_t index)
{
	//if (streams.find(index) == streams.end())
	//	return {};
	return streams[index].formats;
}

bool VideoSourceReader::SetCurrentFormat(uint32_t index, int formatIndex)
{
	if (streams.find(index) == streams.end())
		return false;
	auto &stream = streams[index];

	EnterCriticalSection(&m_critsec);

	IMFMediaType *pType;
	auto hr = m_pReader->GetNativeMediaType(index, formatIndex, &pType);
	if (SUCCEEDED(hr))
	{
		//stream.currentFormatIndex = formatIndex;
		hr = m_pReader->SetCurrentMediaType(index, nullptr, pType);
		MFHelper::GetAttributeSize(pType, stream.frameWidth, stream.frameHeight);
		MFHelper::GetDefaultStride(pType, stream.defaultStride);
	}
	System::SafeRelease(&pType);
	LeaveCriticalSection(&m_critsec);
	return SUCCEEDED(hr);
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

void VideoSourceReader::EnumerateStreams()
{
	if (m_pReader == nullptr)
		return;

	EnterCriticalSection(&m_critsec);

	m_pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, TRUE);

	IMFMediaType *pType;
	GUID majorType;
	auto i = 0;
	while (m_pReader->GetNativeMediaType(i, 0, &pType) == S_OK)
	{
		pType->GetMajorType(&majorType);
		if (IsEqualGUID(majorType, MFMediaType_Video))
		{
			streams[i] = StreamState {};
			EnumerateFormats(i, streams[i].formats);
		}
		System::SafeRelease(&pType);
		i++;
	}

	LeaveCriticalSection(&m_critsec);
}

void VideoSourceReader::EnumerateFormats(uint32_t index, std::vector<VideoFormat> &videoFormats) const
{
	videoFormats.clear();
	if (m_pReader == nullptr)
		return;

	int rate, den, width, height;
	for (auto i = 0;; i++)
	{
		IMFMediaType *pType;
		auto hr = m_pReader->GetNativeMediaType(index, i, &pType);
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
}

bool VideoSourceReader::StartStream(uint32_t index)
{
	if (streams.find(index) == streams.end())
		return false;

	StopStream(index);

	EnterCriticalSection(&m_critsec);

	streams[index].isRunning = true;
	auto hr = m_pReader->ReadSample(
			index,
			0,
			nullptr,
			nullptr,
			nullptr,
			nullptr
			);
	
	LeaveCriticalSection(&m_critsec);
	return hr==S_OK;
}

bool VideoSourceReader::StopStream(uint32_t index)
{
	if (streams.find(index) == streams.end())
		return false;
	EnterCriticalSection(&m_critsec);
	streams[index].isRunning = false;
	LeaveCriticalSection(&m_critsec);
	while (streams[index].streamOn)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return true;
}

bool VideoSourceReader::IsStreaming(uint32_t index)
{
	if (streams.find(index) == streams.end())
		return false;
	return streams[index].streamOn;
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
	DWORD streamIndex,
	DWORD flags,
	LONGLONG timeStamp/* llTimestamp */,
	IMFSample *pSample      // Can be NULL
	)
{
	
	auto &stream = streams[streamIndex];

	auto hr = S_OK;
	IMFMediaBuffer *pBuffer = nullptr;

	EnterCriticalSection(&m_critsec);

	if (pSample)
	{
		OutputDebugStringW(L"Frame Arrival\n");
		// Get the video frame buffer from the sample.

		hr = pSample->GetBufferByIndex(streamIndex, &pBuffer);

		// Draw the frame.
		if (SUCCEEDED(hr) && stream.fncb !=nullptr)
		{
			std::shared_ptr<IVideoFrame> frame = std::make_shared<VideoBufferLock>(
					pBuffer, timeStamp, stream.defaultStride, stream.frameWidth, stream.frameHeight);
			//Invoke callback handler
			stream.fncb(frame);
		}
	}

	// Request the next frame.
	if (SUCCEEDED(hr) && stream.isRunning)
	{
		hr = m_pReader->ReadSample(
			streamIndex,
			0,
			nullptr,   // actual
			nullptr,   // flags
			nullptr,   // timestamp
			nullptr    // sample
			);
		stream.streamOn = true;
	}
	else
	{
		stream.isRunning = false;
		stream.streamOn = false;
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

void VideoSourceReader::RegisterReaderCallback(uint32_t index, const ReaderCallbackFn& fn)
{
	if (streams.find(index) == streams.end())
		return;
	EnterCriticalSection(&m_critsec);
	streams[index].fncb = fn;
	LeaveCriticalSection(&m_critsec);
}
