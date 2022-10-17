#include "pch.h"
#include "ShaderDX12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

#include <AssetManager.h>

namespace nv::graphics
{
    using namespace asset;

    struct DxcBlob : public IDxcBlob
    {
        // Inherited via IDxcBlob
        virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
        {
            return E_NOTIMPL;
        }

        virtual ULONG __stdcall AddRef(void) override
        {
            return 0;
        }

        virtual ULONG __stdcall Release(void) override
        {
            return 0;
        }

        virtual LPVOID __stdcall GetBufferPointer(void) override
        {
            return (LPVOID)mByteCode.pShaderBytecode;
        }

        virtual SIZE_T __stdcall GetBufferSize(void) override
        {
            return mByteCode.BytecodeLength;
        }

        D3D12_SHADER_BYTECODE mByteCode;
    };

    void ShaderDX12::LoadFromCompiledFile(const wchar_t* filename)
    {
        //D3DReadFileToBlob(filename, &mBlob);
    }

    void ShaderDX12::Load()
    {
        Asset* asset = gpAssetManager->GetAsset(mDesc.mShader);
        if (asset->GetState() != STATE_LOADED)
            gpAssetManager->LoadAsset(mDesc.mShader, nullptr, true);

        DxcBlob* blob = Alloc<DxcBlob>();
        blob->mByteCode = {
                .pShaderBytecode = asset->GetData(),
                .BytecodeLength = asset->Size()
        };

        mDxcBlob = blob;
    }

    D3D12_SHADER_BYTECODE ShaderDX12::GetByteCode() const
    {
        if (!mBlob)
        {
            return
            {
                .pShaderBytecode = mDxcBlob->GetBufferPointer(),
                .BytecodeLength = mDxcBlob->GetBufferSize()
            };
        }

        return 
        {  
            .pShaderBytecode = mBlob->GetBufferPointer(), 
            .BytecodeLength  = mBlob->GetBufferSize() 
        };
    }

    ShaderDX12::~ShaderDX12()
    {
        Free(mDxcBlob);
    }
}


