#pragma once

#include <Lib/Handle.h>
#include <Renderer/Mesh.h>
#include <Engine/Component.h>
#include <Components/Material.h>

namespace nv::graphics::components
{
    struct Renderable : public ecs::IComponent
    {
        Handle<Mesh>        mMesh;
        Handle<Material>    mMaterial;
    };
}