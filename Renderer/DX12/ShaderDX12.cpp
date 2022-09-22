#include "pch.h"
#include "ShaderDX12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

#include <AssetManager.h>

namespace nv::graphics
{
    using namespace asset;

    void ShaderDX12::LoadFromCompiledFile(const wchar_t* filename)
    {
        //D3DReadFileToBlob(filename, &mBlob);
    }

    void ShaderDX12::Load()
    {
        Asset* asset = gpAssetManager->GetAsset(mDesc.mShader);
        if (asset->GetState() != STATE_LOADED)
            gpAssetManager->LoadAsset(mDesc.mShader, nullptr, true);
    }

    D3D12_SHADER_BYTECODE ShaderDX12::GetByteCode() const
    {
        if (!mBlob)
        {
            Asset* asset = gpAssetManager->GetAsset(mDesc.mShader);
            return
            {
                .pShaderBytecode = asset->GetData(),
                .BytecodeLength = asset->Size()
            };
        }

        return 
        {  
            .pShaderBytecode = mBlob->GetBufferPointer(), 
            .BytecodeLength  = mBlob->GetBufferSize() 
        };
    }
}


