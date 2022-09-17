#include "pch.h"
#include "GPUProfile.h"

#include <NovaConfig.h>

#if NV_RENDERER_DX12

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <Windows.h>
#include <d3d12.h>
#include <pix3.h>
#include <DX12/ContextDX12.h>

#endif //NV_RENDERER_DX12

namespace nv::graphics
{
#if NV_RENDERER_DX12
    void GPUBeginEvent(Context* ctx, const char* str)
    {
        auto context = (ContextDX12*)ctx;
        PIXBeginEvent(context->GetCommandList(), PIX_COLOR_DEFAULT, str);
    }

    void GPUEndEvent(Context* ctx)
    {
        auto context = (ContextDX12*)ctx;
        PIXEndEvent(context->GetCommandList());
    }

    GPUScopedEvent::GPUScopedEvent(Context* ctx, const char* str):
        mContext(ctx)
    {
        auto context = (ContextDX12*)ctx;
        PIXBeginEvent(context->GetCommandList(), PIX_COLOR_DEFAULT, str);
    }

    GPUScopedEvent::~GPUScopedEvent()
    {
        auto context = (ContextDX12*)mContext;
        PIXEndEvent(context->GetCommandList());
    }
#else
    void GPUBeginEvent(Context* ctx, const char* str)
    {
    }

    void GPUEndEvent(Context* ctx)
    {
    }

    GPUScopedEvent::GPUScopedEvent(Context* ctx, const char* str) :
        mContext(ctx)
    {
    }

    GPUScopedEvent::~GPUScopedEvent()
    {
    }
#endif
}