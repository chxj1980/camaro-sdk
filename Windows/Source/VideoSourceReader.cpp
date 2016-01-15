#include "VideoSourceReader.h"

#include <string>
//#include <sstream>
#include "MFHelper.h"
#include "System.h"
//#include <iostream>
#include <thread>

#include "VideoBufferLock.h"
#include "VideoSourceProxy.h"
#include "MSource.h"
//#include <ks.h>
//#include <ksmedia.h>
//#include <ksproxy.h>

using namespace TopGear;
using namespace Win;

//-------------------------------------------------------------------
//  CreateInstance
//
//-------------------------------------------------------------------

std::vector<std::shared_ptr<IVideoStream>> VideoSourceReader::CreateVideoStreams(std::shared_ptr<IGenericVCDevice> &pSource)
{
	std::shared_ptr<VideoSourceReader> pPlayer(new VideoSourceReader);
	if (pPlayer == nullptr)
		return{};// E_OUTOFMEMORY
	
	auto msource = std::dynamic_pointer_cast<MSource>(pSource->GetSource());
	if (msource==nullptr)
		return{};

	std::vector<std::shared_ptr<IVideoStream>> list;
	auto hr = pPlayer->OpenMediaSource(msource->GetMediaSource());
	if (SUCCEEDED(hr))
	{
		pPlayer->EnumerateStreams();
		for (auto s : pPlayer->streams)
			list.emplace_back(std::make_shared<VideoSourceProxy>(
				std::static_pointer_cast<IMultiVideoSource>(pPlayer), s.first));
	}
	return list;
}

std::shared_ptr<IVideoStream> VideoSourceReader::CreateVideoStream(std::shared_ptr<IGenericVCDevice> &pSource)
{
	std::shared_ptr<VideoSourceReader> pPlayer(new VideoSourceReader);
	if (pPlayer == nullptr)
		return{};// E_OUTOFMEMORY

	auto msource = std::dynamic_pointer_cast<MSource>(pSource->GetSource());
	if (msource == nullptr)
		return{};
	auto hr = pPlayer->OpenMediaSource(msource->GetMediaSource());
	
	std::shared_ptr<IVideoStream> reader;
	if (SUCCEEDED(hr))
	{
		pPlayer->EnumerateStreams(true);
		for (auto s : pPlayer->streams)
		{
			reader = std::make_shared<VideoSourceProxy>(
				std::static_pointer_cast<IMultiVideoSource>(pPlayer),
				s.first);
			break;
		}
	}
	return reader;
}


//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

VideoSourceReader::VideoSourceReader() :
	m_nRefCount(1),
	m_pReader(nullptr)
{
	//InitializeCriticalSection(&m_critsec);
}

void VideoSourceReader::NotifyError(HRESULT hr)
{
	error.store(hr);
}

//-------------------------------------------------------------------
//  destructor
//-------------------------------------------------------------------

VideoSourceReader::~VideoSourceReader()
{
	CloseDevice();

	//m_draw.DestroyDevice();

	//DeleteCriticalSection(&m_critsec);
}

//-------------------------------------------------------------------
//  CloseDevice
//
//  Releases all resources held by this object.
//-------------------------------------------------------------------

HRESULT VideoSourceReader::CloseDevice()
{
	//EnterCriticalSection(&m_critsec);
	std::lock_guard<std::mutex> lg(mtx);
	System::SafeRelease(&m_pReader);

	//LeaveCriticalSection(&m_critsec);
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

	std::lock_guard<std::mutex> lg(mtx);

	IMFMediaType *pType;
	auto hr = m_pReader->GetNativeMediaType(index, formatIndex, &pType);
	if (SUCCEEDED(hr))
	{
		stream.currentFormatIndex = formatIndex;
		hr = m_pReader->SetCurrentMediaType(index, nullptr, pType);
		//MFHelper::GetAttributeSize(pType, stream.frameWidth, stream.frameHeight);
		MFHelper::GetDefaultStride(pType, stream.defaultStride);
	}
	System::SafeRelease(&pType);
	return SUCCEEDED(hr);
}

HRESULT VideoSourceReader::OpenMediaSource(IMFMediaSource* pSource)
{
	IMFAttributes *pAttributes = nullptr;

	// Release the current device, if any.
	auto hr = CloseDevice();

	std::lock_guard<std::mutex> lg(mtx);
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
	return hr;
}

void VideoSourceReader::EnumerateStreams(bool onlyFirst)
{
	if (m_pReader == nullptr)
		return;

	std::lock_guard<std::mutex> lg(mtx);

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
			if (onlyFirst)
				break;
		}
		System::SafeRelease(&pType);
		i++;
	}
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

	std::unique_lock<std::mutex> lk(mtx);

	//Stop
	streams[index].isRunning = false;
	auto result = cv.wait_for(lk, std::chrono::milliseconds(100)) != std::cv_status::timeout;
	streams[index].streamOn = false;

	streams[index].isRunning = true;
	auto hr = m_pReader->ReadSample(
			index,
			0,
			nullptr,
			nullptr,
			nullptr,
			nullptr
			);
	if (FAILED(hr))
		NotifyError(hr);
	return hr==S_OK;
}

bool VideoSourceReader::StopStream(uint32_t index)
{
	if (streams.find(index) == streams.end())
		return false;
	std::unique_lock<std::mutex> lck(mtx);
	streams[index].isRunning = false;
	auto result = cv.wait_for(lck, std::chrono::milliseconds(100))!=std::cv_status::timeout;
	result = result && error == 0;
	streams[index].streamOn = false;
	return result;
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
		{ nullptr },
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

	std::lock_guard<std::mutex> lg(mtx);

	if (pSample)
	{
		//OutputDebugStringW(L"Frame Arrival\n");
		// Get the video frame buffer from the sample.

		hr = pSample->GetBufferByIndex(0, &pBuffer);

		// Draw the frame.
		if (SUCCEEDED(hr) && stream.fncb !=nullptr)
		{
			std::shared_ptr<IVideoFrame> frame = std::make_shared<VideoBufferLock>(
					pBuffer, timeStamp, stream.defaultStride, stream.formats[stream.currentFormatIndex]);
			//Invoke callback handler
			stream.fncb(frame);
		}
	}

	// Request the next frame.
	if (stream.isRunning)
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
	//else
	//{
	//	stream.isRunning = false;
	//	stream.streamOn = false;
	//}

	if (FAILED(hr))
	{
		NotifyError(hr);
	}
	System::SafeRelease(&pBuffer);
	if (!stream.isRunning)
		cv.notify_one();
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
	std::lock_guard<std::mutex> lg(mtx);
	streams[index].fncb = fn;
}
