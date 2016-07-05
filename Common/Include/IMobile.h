#pragma once

namespace TopGear
{
    class IMobile
    {
    public:
        virtual ~IMobile() = default;
        virtual void SyncTag() = 0;
        virtual bool IsSteady() = 0;
    };
}

