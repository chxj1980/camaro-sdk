#pragma once
#include <vector>
#include "IVideoFrame.h"

namespace TopGear
{
	class IProcessor
	{
	public:
		virtual ~IProcessor() = default;
		virtual std::vector<IVideoFrameRef> Process(std::vector<IVideoFrameRef> &source) = 0;
	};

	class IProcessable
	{
	public:
		virtual ~IProcessable() = default;
		virtual void RegisterProcessor(std::shared_ptr<IProcessor> &processor) = 0;
	};
}
