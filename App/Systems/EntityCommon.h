#pragma once

#include <Renderer/CommonDefines.h>
#include <Engine/Transform.h>
#include <Lib/Handle.h>
#include <Engine/EntityComponent.h>

namespace nv
{
    Handle<ecs::Entity> CreateEntity(graphics::ResID mesh, graphics::ResID mat, const Transform& transform = Transform());
}