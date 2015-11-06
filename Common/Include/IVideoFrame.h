#pragma once

#ifndef _IVIDEOFRAME_H
#define _IVIDEOFRAME_H

#include <cstdint>

#ifdef _WIN32
#include <winsock.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif
#include <vector>
#include <memory>
#include <functional>

#ifndef DEPRECATED
#ifdef __GNUC__
#define DEPRECATED(type) type //__attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(type) type //__declspec(deprecated) type
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(type) type
#endif
#endif

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

		virtual uint16_t GetFrameIdx() const = 0;    //frame index, used to check frame drops
		virtual timeval GetTimestamp() const = 0;    //timestamp
		virtual uint32_t GetLength() const = 0;      //length(bytes) of actual frame buffer
		virtual uint32_t GetExtraLength() const = 0;      //length(bytes) of extra data
	};

	typedef std::shared_ptr<IVideoFrame> IVideoFrameRef;
	typedef std::function<void(std::vector<IVideoFrameRef> &)> VideoFrameCallbackFn;

    DEPRECATED(class IVideoFrameCallback);

    class IVideoFrameCallback
    {
    public:
        virtual ~IVideoFrameCallback() = default;
        virtual void OnFrame(std::vector<IVideoFrameRef> &frames) = 0;
    };
}

#endif

