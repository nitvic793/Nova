#include "pch.h"
#include "ShaderDX12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

namespace nv::graphics
{
    void ShaderDX12::LoadFromCompiledFile(const wchar_t* filename)
    {
        D3DReadFileToBlob(filename, &mBlob);
    }

    D3D12_SHADER_BYTECODE ShaderDX12::GetByteCode() const
    {
        return 
        {  
            .pShaderBytecode = mBlob->GetBufferPointer(), 
            .BytecodeLength  = mBlob->GetBufferSize() 
        };
    }
}


