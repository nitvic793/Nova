#pragma once

#include <Renderer/Shader.h>
#include <Asset.h>
#include <ostream>

namespace nv::asset
{
    struct ShaderConfigData
    {
        graphics::shader::Type          mShaderType;
        graphics::shader::ShaderModel   mShaderModel;
    };

    struct ShaderConfig
    {
        std::unordered_map<std::string, ShaderConfigData> mConfigMap;
    };

    void LoadShaderConfigData(std::istream& i);
    void LoadShaderConfigDataBinary(std::istream& i);
    void ExportShaderConfigDataBinary(std::ostream& o);

    class ShaderAsset
    {
        using ShaderDesc = graphics::ShaderDesc;

    public:
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, const char* name, std::ostream& ostream);

        const ShaderDesc& GetData() const { return mData; }
        ~ShaderAsset() {};

    private:
        ShaderDesc                mData = {};
    };

    extern ShaderConfig gShaderConfig;
}