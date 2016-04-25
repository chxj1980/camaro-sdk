#pragma once

#ifdef _WIN32
#include <winsock.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif
#include <cstdint>
#include <memory>

#include "VideoFormat.h"

namespace TopGear
{

	class IVideoFrame
	{
	public:
		virtual ~IVideoFrame() = default;
		virtual int LockBuffer(
			uint8_t  **ppData,    // Receives a pointer to the start of scan line 0.
			uint32_t  *pStride,          // Receives the actual stride.
			uint8_t  **ppExtra = nullptr  // Receives a pointer to extra data.
			) = 0;
		virtual void UnlockBuffer() = 0;
		virtual VideoFormat GetFormat() const = 0;
        virtual uint64_t GetFrameIndex() const = 0;    //frame index, used to check frame drops
		virtual timeval GetTimestamp() const = 0;    //timestamp
		virtual uint32_t GetLength() const = 0;      //length(bytes) of actual frame buffer
		virtual uint32_t GetExtraLength() const = 0;      //length(bytes) of extra data
	};

	typedef std::shared_ptr<IVideoFrame> IVideoFramePtr;
}


