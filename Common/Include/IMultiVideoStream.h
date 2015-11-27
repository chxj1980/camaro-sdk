#pragma once
#include "IVideoStream.h"

namespace TopGear
{
	class IMultiVideoStream
	{
	public:
		virtual ~IMultiVideoStream() = default;

		virtual const std::vector<std::shared_ptr<IVideoStream>> &GetStreams() const = 0;
		virtual bool SelectStream(int index) = 0;
		virtual bool SelectStream(const std::shared_ptr<IVideoStream> &vs) = 0;
		virtual int GetCurrentStream(std::shared_ptr<IVideoStream> &current) = 0;
		virtual void StartStreams() = 0;
		virtual void StopStreams() = 0;
	};
}
