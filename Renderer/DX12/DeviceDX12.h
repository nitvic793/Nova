#pragma once

#include <Renderer/Device.h>
#include <wrl/client.h>

struct IDXGIFactory4;
struct ID3D12Device;
struct IDXGISwapChain4;
struct ID3D12CommandQueue;

enum DXGI_FORMAT;

namespace nv::graphics
{
    class Window;

    class DeviceDX12 : public Device
    {
    public:
        bool Init(Window& window) override;
        void Present(); //Temp
        ~DeviceDX12();

    private:
        bool InitSwapChain(Window& window, DXGI_FORMAT format);

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        ComPtr<IDXGIFactory4>       mDxgiFactory;
        ComPtr<ID3D12Device>        mDevice;
        ComPtr<IDXGISwapChain4>     mSwapChain;
        ComPtr<ID3D12CommandQueue>  mCommandQueue;

        friend class RendererDX12;
    };
}