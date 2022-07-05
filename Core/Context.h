#pragma once

namespace nv
{
    class MemTracker;
    class SystemAllocator;

    struct Context
    {
        MemTracker*         mpMemTracker;
        static Context*     gPtr;
    };

    extern Context gContext;

    void InitContext(Context* pContext = Context::gPtr);
    void DestroyContext(Context* pContext = Context::gPtr);

    constexpr void ApplyContext(Context* pContext) 
    { 
        Context::gPtr = pContext; 
        gContext = *pContext; 
    }
}