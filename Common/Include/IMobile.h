#pragma once

namespace TopGear
{
    class IMobile
    {
    public:
        virtual ~IMobile() = default;
        virtual void StartMove()  = 0;
        virtual void StopMove()  = 0;
        virtual bool IsSteady() = 0;
    };
}

