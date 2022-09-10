#include "pch.h"
#include "PipelineStateDX12.h"
#include <d3d12.h>

namespace nv::graphics
{ 

ID3D12PipelineState* nv::graphics::PipelineStateDX12::GetPSO() const
{
    return mPipelineState.Get();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState>& PipelineStateDX12::GetPipelineCom()
{
    return mPipelineState;
}

}