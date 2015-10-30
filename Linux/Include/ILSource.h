#pragma once
#include <utility>
#include <string>

namespace TopGear
{
    namespace Linux
	{

        typedef std::pair<std::string, int> SourcePair;

        class ILSource
		{
		public:
            virtual ~ILSource() = default;
            virtual int GetSource() = 0;
		};
	}
}
