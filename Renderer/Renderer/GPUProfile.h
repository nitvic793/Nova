#pragma once

#include <Renderer/Context.h>

namespace nv::graphics
{
    void GPUBeginEvent(Context* ctx, const char* str);
    void GPUEndEvent(Context* ctx);

    class GPUScopedEvent
    {
    public:
        GPUScopedEvent(Context* ctx, const char* str);
        ~GPUScopedEvent();

    private:
        Context* mContext;
    };
}