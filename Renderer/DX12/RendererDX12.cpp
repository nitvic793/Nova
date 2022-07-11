#include "pch.h"
#include "RendererDX12.h"
#include <DX12/DeviceDX12.h>
#include <DX12/DirectXIncludes.h>
#include <DX12/WindowDX12.h>

#include <dxgidebug.h>

namespace nv::graphics
{
    void RendererDX12::Init(Window& window)
    {
        mDevice = ScopedPtr<Device, true>((Device*)Alloc<DeviceDX12>());
        mDevice->Init(window);
    }

    void RendererDX12::Destroy()
    {
    }

    void RendererDX12::Present()
    {
        auto device = (DeviceDX12*)mDevice.Get();
        device->Present();
    }

    void ReportLeaksDX12()
    {
#ifdef _DEBUG
        IDXGIDebug1* pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            pDebug->Release();
        }
#endif
    }
}

