#pragma once

namespace TopGear
{
    class IIrisControl
	{
	public:
        virtual int GetIris(float &ratio) = 0;
        virtual int SetIris(float ratio) = 0;
        virtual int SetIrisOffset(int offset) = 0;
    };
}