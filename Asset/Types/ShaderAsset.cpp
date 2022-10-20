#include "pch.h"
#include "ShaderAsset.h"

#include <Lib/Util.h>
#include <AssetManager.h>

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

#include <d3d12.h>
#include <dxcapi.h>
#include <atlbase.h>
#include <array>

constexpr auto DXCOMPILER_DLL = L"dxcompiler.dll";

namespace nv
{
    using namespace graphics;

    static const nv::HashMap<shader::Type, std::string> sgShaderTypeMap =
    {
        { shader::PIXEL,    "PixelShader"   },
        { shader::VERTEX,   "VertexShader"  },
        { shader::COMPUTE,  "ComputeShader" },
        { shader::LIB,      "Lib"           }
    };

    static const nv::HashMap<std::string, shader::Type> sgShaderTypeMapStr =
    {
        { "PixelShader",     shader::PIXEL,     },
        { "VertexShader",    shader::VERTEX,    },
        { "ComputeShader",   shader::COMPUTE,   },
        { "Lib",             shader::LIB,       }
    };

    static const nv::HashMap<shader::ShaderModel, std::string> sgShaderModelMap =
    {
        { shader::SM_6_5,   "6_5" },
        { shader::SM_6_6,   "6_6" },
        { shader::SM_6_7,   "6_7" }
    };

    static const nv::HashMap<std::string, shader::ShaderModel> sgShaderModelMapStr =
    {
        { "6_5", shader::SM_6_5 },
        { "6_6", shader::SM_6_6 },
        { "6_7", shader::SM_6_7 }
    };

    static const nv::HashMap <shader::Type, std::wstring> sgShaderProfileTypeMap =
    {
        { shader::PIXEL,    L"ps" },
        { shader::VERTEX,   L"vs" },
        { shader::COMPUTE,  L"cs" },
        { shader::LIB,      L"lib" }
    };

    static const nv::HashMap<shader::ShaderModel, std::wstring> sgShaderModelProfileMap =
    {
        { shader::SM_6_5,   L"6_5" },
        { shader::SM_6_6,   L"6_6" },
        { shader::SM_6_7,   L"6_7" }
    };

    static const std::wstring GetTargetProfile(shader::Type type, shader::ShaderModel sm)
    {
        const auto& typeStr = sgShaderProfileTypeMap.at(type);
        const auto& smStr = sgShaderModelProfileMap.at(sm);
        const std::wstring profile = typeStr + L"_" + smStr;
        return profile;
    }
}

namespace cereal
{
    using namespace nv::graphics;
    using namespace nv::asset;

    template<class Archive>
    void save(Archive& archive, ShaderConfigData const& h)
    {
        archive(make_nvp("Type", nv::sgShaderTypeMap.at(h.mShaderType)));
        archive(make_nvp("ShaderModel", nv::sgShaderModelMap.at(h.mShaderModel)));
    }

    template<class Archive>
    void load(Archive& archive, ShaderConfigData& config)
    {
        std::string shaderModel;
        std::string shaderType;

        archive(make_nvp("Type", shaderType));
        archive(make_nvp("ShaderModel", shaderModel));

        config.mShaderModel = nv::sgShaderModelMapStr.at(shaderModel);
        config.mShaderType = nv::sgShaderTypeMapStr.at(shaderType);
    }
}

namespace nv::asset
{
    template<typename T>
    using CComPtr = ATL::CComPtr<T>;

    ShaderConfig gShaderConfig;

    class ShaderCompiler
    {
    public:
        CComPtr<IDxcBlob> Compile(const AssetData& data, const ShaderConfigData& config, const char* name)
        {
            static CComPtr<IDxcUtils> pUtils;
            static CComPtr<IDxcIncludeHandler> pDefaultIncludeHandler;
            static CComPtr<IDxcLibrary> library;
            static CComPtr<IDxcCompiler> compiler;

            class CustomIncludeHandler : public IDxcIncludeHandler
            {
            public:
                HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
                {
                    *ppIncludeSource = nullptr;
                    CComPtr<IDxcBlobEncoding> pEncoding;
                    std::string path = ToString(pFilename);
                    constexpr auto toErase = "./";
                    path.erase(0,2);
                    auto pos = path.find("../");
                    if (pos != std::string::npos)
                    {
                        path.erase(0, pos + 3);
                        path = "Shaders/" + path;
                    }

                    const auto id = AssetID{ ASSET_SHADER, ID(path.c_str()) };
                    auto asset = gpAssetManager->GetAsset(id);
                    if (asset == nullptr)
                    {
                        return S_FALSE;
                    }

                    if (asset->GetState() == STATE_UNLOADED)
                    {
                        gpAssetManager->LoadAsset(id, nullptr, true);
                    }

                    const auto& data = asset->GetAssetData();

                    if (mIncludedFiles.find(path) != mIncludedFiles.end())
                    {
                        // Return empty string blob if this file has been included before
                        static const char nullStr[] = " ";
                        pUtils->CreateBlob(nullStr, ARRAYSIZE(nullStr), CP_UTF8, &pEncoding);
                        *ppIncludeSource = pEncoding.Detach();
                        return S_OK;
                    }

                    auto hr = pUtils->CreateBlobFromPinned(data.mData, (UINT)data.mSize, CP_UTF8, &pEncoding);
                    if (SUCCEEDED(hr))
                    {
                        mIncludedFiles.insert(path);
                        *ppIncludeSource = pEncoding.Detach();
                    }
                    else
                    {
                        *ppIncludeSource = nullptr;
                    }
                    return hr;
                }

                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
                {
                    return pDefaultIncludeHandler->QueryInterface(riid, ppvObject);
                }

                ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
                ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

            private:
                std::unordered_set<std::string> mIncludedFiles;

            };

            CustomIncludeHandler includeHandler;

            HRESULT hr;
            if (!pUtils)
            {
                DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
                pUtils->CreateDefaultIncludeHandler(&pDefaultIncludeHandler);
                hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
                hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
            }

            uint32_t codePage = CP_UTF8;
            CComPtr<IDxcBlobEncoding> sourceBlob;
            hr = library->CreateBlobWithEncodingFromPinned(data.mData, (UINT)data.mSize, codePage, &sourceBlob);

            constexpr auto mainEntry = L"main";
            const std::wstring srcName = ToWString(name);
            const std::wstring profile = GetTargetProfile(config.mShaderType, config.mShaderModel);

            CComPtr<IDxcOperationResult> result;

            std::array<const wchar_t*, 2> args = { L"-Zi", L"-Qembed_debug"};

            hr = compiler->Compile(
                sourceBlob, // pSource
                srcName.c_str(), // pSourceName
                config.mShaderType == shader::LIB ? L"" : mainEntry,  // pEntryPoint
                profile.c_str(), // pTargetProfile
                args.data(), (UINT)args.size(), // pArguments, argCount
                NULL, 0, // pDefines, defineCount
                &includeHandler, // pIncludeHandler
                &result); // ppResult
            if (SUCCEEDED(hr))
                result->GetStatus(&hr);

            if (FAILED(hr))
            {
                if (result)
                {
                    CComPtr<IDxcBlobEncoding> errorsBlob;
                    hr = result->GetErrorBuffer(&errorsBlob);
                    if (SUCCEEDED(hr) && errorsBlob)
                    {
                        wprintf(L"Compilation failed with errors:\n%hs\n",
                            (const char*)errorsBlob->GetBufferPointer());
                    }
                }
                // Handle compilation error...
            }

            CComPtr<IDxcBlob> code;
            result->GetResult(&code);

            return code;
        }
    };

    void ShaderAsset::Deserialize(const AssetData& data)
    {
        
    }

    void ShaderAsset::Export(const AssetData& data, const char* name, std::ostream& ostream)
    {
        static ShaderCompiler compiler;

        if (gShaderConfig.mConfigMap.find(name) == gShaderConfig.mConfigMap.end())
        {
            ostream.write((const char*)data.mData, data.mSize);
            return;
        }

        const auto& config = gShaderConfig.mConfigMap.at(name);
        CComPtr<IDxcBlob> blob = compiler.Compile(data, config, name);

        auto* buffer = reinterpret_cast<const char*>(blob->GetBufferPointer());
        auto  size = blob->GetBufferSize();
        ostream.write(buffer, size);
    }

    void asset::LoadShaderConfigData(std::istream& i)
    {
        cereal::JSONInputArchive archive(i);
        archive(gShaderConfig.mConfigMap);
    }

    void asset::LoadShaderConfigDataBinary(std::istream& i)
    {
        cereal::BinaryInputArchive archive(i);
        archive(gShaderConfig.mConfigMap);
    }

    void asset::ExportShaderConfigDataBinary(std::ostream& o)
    {
        cereal::BinaryOutputArchive archive(o);
        archive(cereal::make_nvp("Shaders", gShaderConfig.mConfigMap));
    }
}

