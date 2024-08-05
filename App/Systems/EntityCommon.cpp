#include "pch.h"
#include "EntityCommon.h"

#include <Renderer/ResourceManager.h>
#include <Components/Renderable.h>
#include <Animation/Animation.h>
#include <DebugUI/DebugUIPass.h>
#include <Math/Collision.h>
#include <Engine/Transform.h>
#include <Engine/Instance.h>

namespace nv
{
    using namespace ecs;
    using namespace math;
    using namespace graphics;
    using namespace graphics::animation;

    Handle<Entity> CreateEntity(graphics::ResID mesh, graphics::ResID mat, const char* pDebugName /*= nullptr*/, const Transform& transform /*= Transform()*/)
    {
        Handle<MaterialInstance> matHandle = gResourceManager->GetMaterialHandle(mat);
        Handle<Mesh> meshHandle = gResourceManager->GetMeshHandle(mesh);
        if (meshHandle.IsNull() && mesh != RES_ID_NULL)
        {
            meshHandle = gResourceManager->CreateMeshAsync(mesh);
        }

        Mesh* pMesh = gResourceManager->GetMesh(meshHandle);

        Handle<Entity> e = ecs::gEntityManager.Create(Null<Entity>(), pDebugName);
        Entity* entity = gEntityManager.GetEntity(e);

        entity->AttachTransform(transform);
        auto pTransform = entity->Add<PrevTransform>();
        *pTransform = *(PrevTransform*)&transform;
        auto renderable = entity->Add<components::Renderable>();
        renderable->mMaterial = matHandle;
        renderable->mMesh = meshHandle;

        if (!meshHandle.IsNull() && pMesh)
        {
            const bool bHasBones = pMesh->HasBones();
            renderable->mFlags = (components::RenderableFlags) (bHasBones ? components::RENDERABLE_FLAG_ANIMATED | components::RENDERABLE_FLAG_DYNAMIC : components::RENDERABLE_FLAG_DYNAMIC);
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

    void FramePreSystem::Init()
    {
        gContext.mpInstance->Wait(); // Wait for renderer to initialize.
    }

    void FramePreSystem::Update(float deltaTime, float totalTime)
    {
        auto positions = ecs::gComponentManager.GetComponents<Position>();
        auto scales = ecs::gComponentManager.GetComponents<Scale>();
        auto rotations = ecs::gComponentManager.GetComponents<Rotation>();
        auto prevTransforms = ecs::gComponentManager.GetComponents<PrevTransform>();

        for (size_t i = 0; i < positions.Size(); ++i)
        {
            auto pos = &positions[i];
            auto scale = &scales[i];
            auto rotation = &rotations[i];

            PrevTransform prevTransform = { pos->mPosition, rotation->mRotation, scale->mScale };
            prevTransforms[i] = prevTransform;
        }
    }

    void FramePreSystem::Destroy()
    {
    }

}
