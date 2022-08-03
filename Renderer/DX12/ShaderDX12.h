#pragma once

#include <Renderer/Shader.h>

struct D3D12_SHADER_BYTECODE;
struct ID3D10Blob;
using ID3DBlob = ID3D10Blob;

namespace nv::graphics
{
    class ShaderDX12 : public Shader
    {
    public:
        ShaderDX12() :
            Shader({}),
            mBlob(nullptr)
        {}

        ShaderDX12(const ShaderDesc& desc) :
            Shader(desc),
            mBlob(nullptr)
        {}

        void LoadFromCompiledFile(const wchar_t* filename);
        D3D12_SHADER_BYTECODE GetByteCode() const;

    private:
        ID3DBlob* mBlob;
    };
}