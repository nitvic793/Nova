#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace nv::graphics
{
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    struct AccelerationStructureBuffers
    {
        ComPtr<ID3D12Resource> mScratch;
        ComPtr<ID3D12Resource> mResult;
        ComPtr<ID3D12Resource> mInstanceDesc;
    };


}