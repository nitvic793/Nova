#include "pch.h"
#include "EntityCommon.h"

#include <Renderer/ResourceManager.h>
#include <Components/Renderable.h>

namespace nv
{
    using namespace ecs;
    using namespace math;
    using namespace graphics;

    Handle<Entity> CreateEntity(graphics::ResID mesh, graphics::ResID mat, const char* pDebugName /*= nullptr*/, const Transform& transform /*= Transform()*/)
    {
        Handle<Material> matHandle = gResourceManager->GetMaterialHandle(mat);
        Handle<Mesh> meshHandle = gResourceManager->GetMeshHandle(mesh);

        Handle<Entity> e = ecs::gEntityManager.Create(Null<Entity>(), pDebugName);
        Entity* entity = gEntityManager.GetEntity(e);

        entity->AttachTransform(transform);
        auto renderable = entity->Add<components::Renderable>();
        renderable->mMaterial = matHandle;
        renderable->mMesh = meshHandle;
        return e;
    }

}
