#include "pch.h"
#include "ShaderAsset.h"

#include <cereal/cereal.hpp>
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
    void save(Archive& archive, shader::Type const& type)
    {
        archive(nv::sgShaderTypeMap[type]);
    }

    template<class Archive>
    void load(Archive& archive, shader::Type& type)
    {
        std::string shaderType;
        archive(shaderType);
        type = nv::sgShaderTypeMapStr[shaderType];
    }

    template<class Archive>
    void save(Archive& archive, shader::ShaderModel const& sm)
    {
        archive(nv::sgShaderModelMap[sm]);
    }

    template<class Archive>
    void load(Archive& archive, shader::ShaderModel& sm)
    {
        std::string shaderModel;
        archive(shaderModel);
        sm = nv::sgShaderModelMapStr[shaderModel];
    }

    template<class Archive>
    void serialize(Archive& archive, ShaderConfigData& h)
    {
        archive(make_nvp("Type", h.mShaderType));
        archive(make_nvp("ShaderModel", h.mShaderModel));
    }

    template<class Archive>
    void serialize(Archive& archive, ShaderConfig& h)
    {
        archive(make_nvp("Shaders", h.mConfigMap));
    }
}

namespace nv::asset
{
    void ShaderAsset::Deserialize(const AssetData& data)
    {
        
    }
    void ShaderAsset::Export(const AssetData& data, std::ostream& ostream)
    {
    }
}

