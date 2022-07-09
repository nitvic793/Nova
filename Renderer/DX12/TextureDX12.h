#pragma once

#include <Lib/Handle.h>
#include <Renderer/Texture.h>

namespace nv::graphics
{
    class GPUResource;

    class TextureDX12 : public Texture
    {
    public:

    private:
        Handle<GPUResource> mResourceHandle;
    };
}