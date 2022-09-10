#pragma once

#include <Renderer/Shader.h>
#include <wrl/client.h>

struct D3D12_SHADER_BYTECODE;
struct ID3D10Blob;
using ID3DBlob = ID3D10Blob;
struct IDxcBlobEncoding;

namespace nv::graphics
{
    class ShaderDX12 : public Shader
    {
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

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
        void Load();

        D3D12_SHADER_BYTECODE GetByteCode() const;

    private:
        ComPtr<ID3DBlob> mBlob;
    };
}