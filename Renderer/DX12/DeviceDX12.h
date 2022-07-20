#pragma once

#include <Renderer/Device.h>
#include <wrl/client.h>

struct IDXGIFactory4;
struct ID3D12Device;
struct IDXGISwapChain4;
struct ID3D12CommandQueue;

struct D3D12_RESOURCE_DESC;

enum DXGI_FORMAT;
enum D3D12_RESOURCE_STATE;

namespace D3D12MA
{
    class Allocator;
    struct ALLOCATION_DESC;
}

namespace nv::graphics
{
    class Window;

    class DeviceDX12 : public Device
    {
    public:
        bool Init(Window& window) override;
        void Present() override;
        ~DeviceDX12();

        bool InitSwapChain(const Window& window, const format::SurfaceFormat format) override;

        ID3D12Device* GetDevice() const;
        D3D12MA::Allocator* GetAllocator() const;

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        ComPtr<IDXGIFactory4>       mDxgiFactory;
        ComPtr<ID3D12Device>        mDevice;
        ComPtr<IDXGISwapChain4>     mSwapChain;
        ComPtr<ID3D12CommandQueue>  mCommandQueue;
        ComPtr<D3D12MA::Allocator>  mGpuAllocator;
        friend class RendererDX12;
    };
}