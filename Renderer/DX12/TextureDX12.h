#pragma once

#include <Lib/Handle.h>
#include <Renderer/Texture.h>

namespace nv::graphics
{
    class TextureDX12 : public Texture
    {
    public:
        // Inherited via Texture
        virtual void Create(const TextureDesc& desc) override;
    };
}