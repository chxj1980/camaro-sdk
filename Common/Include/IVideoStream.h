#pragma once
#include "IVideoFrame.h"
#include "VideoFormat.h"

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
#include <vector>
#include <functional>
#include <chrono>

namespace TopGear
{
	class IVideoStream;

    typedef std::function<void(IVideoStream &, std::vector<IVideoFramePtr> &)> VideoFrameCallbackFn;

    typedef std::function<void(IVideoStream &)> TimeoutCallbackFn;

    class IWatch
    {
    public:
        virtual ~IWatch() = default;
        virtual void RegisterTimeoutCallback(const TimeoutCallbackFn &fn, std::chrono::seconds timeout) = 0;
    };

	DEPRECATED(class IVideoFrameCallback);

	class IVideoFrameCallback
	{
	public:
		virtual ~IVideoFrameCallback() = default;
		virtual void OnFrame(IVideoStream &sender, std::vector<IVideoFramePtr> &frames) = 0;
	};

	template<typename testType>
	struct is_function_pointer
	{
		static const bool value =
			std::is_pointer<testType>::value ?
			std::is_function<typename std::remove_pointer<testType>::type>::value :
			false;
	};

	class IVideoStream
	{
	public:
		virtual ~IVideoStream() = default;
		virtual bool StartStream() = 0;
		virtual bool StopStream() = 0;
		virtual bool IsStreaming() const = 0;

		virtual int GetOptimizedFormatIndex(VideoFormat &format, const char *fourcc = "") = 0;
		virtual int GetMatchedFormatIndex(const VideoFormat &format) const = 0;
		virtual const std::vector<VideoFormat> &GetAllFormats() const = 0;
		virtual const VideoFormat &GetCurrentFormat() const = 0;
		virtual bool SetCurrentFormat(uint32_t formatIndex) = 0;
		virtual void RegisterFrameCallback(const VideoFrameCallbackFn &fn) = 0;

		DEPRECATED(virtual void RegisterFrameCallback(IVideoFrameCallback *pCB)) = 0;

		template<class Fn, class T>
		static void RegisterFrameCallback(IVideoStream &stream, Fn&& fn, T *ptr)
		{
			static_assert(std::is_member_function_pointer<Fn>::value, "RegisterFrameCallback need a member function as parameter");
			stream.RegisterFrameCallback(std::bind(fn, ptr, std::placeholders::_1, std::placeholders::_2));
		}

		template<class Fn>
        static void RegisterFrameCallback(IVideoStream &stream, Fn&& fn)
		{
			static_assert(is_function_pointer<Fn>::value, "RegisterFrameCallback need a function as parameter");
			stream.RegisterFrameCallback(std::bind(fn, std::placeholders::_1, std::placeholders::_2));
		}
	};
}
