#pragma once
#include <vector>
#include "IVideoFrame.h"

namespace TopGear
{
	class IProcessable;

	class IProcessor
	{
	public:
		virtual ~IProcessor() = default;
        virtual bool Process(IProcessable &sender, std::vector<IVideoFramePtr> &source) = 0;
	};

	class IProcessable
	{
	public:
		virtual ~IProcessable() = default;
		virtual void Register(std::shared_ptr<IProcessor> &processor) = 0;
		virtual void Notify(std::vector<IVideoFramePtr> &payload) = 0;
	};
}
