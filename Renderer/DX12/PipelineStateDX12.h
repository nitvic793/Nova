#pragma once

#include <Renderer/PipelineState.h>
#include <wrl/client.h>

struct ID3D12PipelineState;

namespace nv::graphics
{
    class PipelineStateDX12 : public PipelineState
    {
    public:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        PipelineStateDX12() :
            PipelineState({})
        {}

        PipelineStateDX12(const PipelineStateDesc& desc) :
            PipelineState(desc)
        {}

        ID3D12PipelineState* GetPSO() const;
        ComPtr<ID3D12PipelineState>& GetPipelineCom();

    private:
        ComPtr<ID3D12PipelineState> mPipelineState;
    };
}