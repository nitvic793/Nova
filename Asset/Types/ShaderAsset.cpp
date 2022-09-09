#include "pch.h"
#include "ShaderAsset.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

namespace nv
{
    using namespace graphics;

    static const nv::HashMap<shader::Type, std::string> sgShaderTypeMap =
    {
        { shader::PIXEL,    "PixelShader" },
        { shader::VERTEX,   "VertexShader" },
        { shader::COMPUTE,  "ComputeShader" }
    };

    static const nv::HashMap<std::string, shader::Type> sgShaderTypeMapStr =
    {
        { "PixelShader",     shader::PIXEL,    },
        { "VertexShader",    shader::VERTEX,   },
        { "ComputeShader",   shader::COMPUTE,   }
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

    template<class Archive>
    void serialize(Archive& archive, ShaderConfig& h)
    {
        archive(make_nvp("Shaders", h.mConfigMap));
    }
}

namespace nv::asset
{
    ShaderConfig gShaderConfig;

    void ShaderAsset::Deserialize(const AssetData& data)
    {
        
    }
    void ShaderAsset::Export(const AssetData& data, std::ostream& ostream)
    {
    }

    void asset::LoadShaderConfigData(std::istream& i)
    {
        cereal::JSONInputArchive archive(i);
        archive(gShaderConfig);
    }

    void asset::ExportShaderConfigData(std::ostream& o)
    {
        cereal::BinaryOutputArchive archive(o);
        archive(gShaderConfig);
    }
}

