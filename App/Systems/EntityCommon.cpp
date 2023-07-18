#include "pch.h"
#include "EntityCommon.h"

#include <Renderer/ResourceManager.h>
#include <Components/Renderable.h>
#include <Animation/Animation.h>
#include <DebugUI/DebugUIPass.h>
#include <Math/Collision.h>

namespace nv
{
    using namespace ecs;
    using namespace math;
    using namespace graphics;
    using namespace graphics::animation;

    Handle<Entity> CreateEntity(graphics::ResID mesh, graphics::ResID mat, const char* pDebugName /*= nullptr*/, const Transform& transform /*= Transform()*/)
    {
        Handle<Material> matHandle = gResourceManager->GetMaterialHandle(mat);
        Handle<Mesh> meshHandle = gResourceManager->GetMeshHandle(mesh);
        Mesh* pMesh = gResourceManager->GetMesh(meshHandle);

        Handle<Entity> e = ecs::gEntityManager.Create(Null<Entity>(), pDebugName);
        Entity* entity = gEntityManager.GetEntity(e);

        entity->AttachTransform(transform);
        auto renderable = entity->Add<components::Renderable>();
        renderable->mMaterial = matHandle;
        renderable->mMesh = meshHandle;
        if (!meshHandle.IsNull())
        {
            const bool bHasBones = pMesh->HasBones();
            renderable->mFlags = bHasBones ? components::RENDERABLE_FLAG_ANIMATED : components::RENDERABLE_FLAG_DYNAMIC;
            if (bHasBones)
            {
                entity->Add<components::AnimationComponent>();
                gAnimManager.Register(e.mHandle, pMesh);
            }

            auto pBox = entity->Add<BoundingBox>();
            *pBox = pMesh->GetBoundingBox();
        }

        return e;
    }

    bool IsDebugUIEnabled()
    {
        return graphics::IsDebugUIEnabled();
    }

}
