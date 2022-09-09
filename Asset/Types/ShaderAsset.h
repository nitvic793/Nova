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

    class ShaderAsset
    {
        using ShaderDesc = graphics::ShaderDesc;

    public:
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, std::ostream& ostream);

        const ShaderDesc& GetData() const { return mData; }
        ~ShaderAsset() {};

    private:
        ShaderDesc                mData = {};
    };
}