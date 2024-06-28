#include "pch.h"

#include <Debug/Profiler.h>
#include "DriverSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Engine/Component.h>
#include <Engine/EventSystem.h>
#include <Engine/JobSystem.h>


#include <Input/Input.h>
#include <Engine/Log.h>
#include <Engine/Instance.h>
#include <Interop/ShaderInteropTypes.h>
#include <DebugUI/DebugUIPass.h>
#include "EntityCommon.h"
#include <Math/Collision.h>
#include <sstream>
#include <fstream>

namespace nv
{
    using namespace ecs;

    constexpr float INPUT_DELAY_DEBUG_PRESS = 1.f;
    static bool sbEnableDebugUI = false;

    static float sDelayTimer = 0.f;

    FrameRecordState mFrameRecordState = FRAME_RECORD_STOPPED;
    ecs::Entity* entity;

    struct FrameData
    {
        std::stringstream mStream;
    };

    std::vector<FrameData> gFrameStack;

    void PushFrame();
    bool PopFrame();

    void DriverSystem::Init()
    {
        using namespace ecs;
        using namespace graphics;
        using namespace components;

        gComponentManager.LoadMetadata();

        const auto setupSky = [&]()
        {
            auto e3 = CreateEntity(RES_ID_NULL, RES_ID_NULL, "Sky");
            auto directionalLight = gEntityManager.GetEntity(e3)->Add<DirectionalLight>();
            directionalLight->Color = float3(0.9f, 0.9f, 0.9f);
            directionalLight->Intensity = 1.f;
            auto skybox = gEntityManager.GetEntity(e3)->Add<SkyboxComponent>();
            skybox->mSkybox = gResourceManager->GetTextureHandle(ID("Textures/SunnyCubeMap.dds"));
            Store(Vector3Normalize(VectorSet(1, -1, 1, 0)), directionalLight->Direction);
        };

        auto entity1 = CreateEntity(ID("Mesh/torus.obj"), ID("Floor"), "Torus");
        auto playerEntity = CreateEntity(ID("Mesh/cube.obj"), ID("Bronze"), "Box");
        auto floor = CreateEntity(ID("Mesh/plane.obj"), ID("Floor"), "Floor");

        setupSky();

        entity = gEntityManager.GetEntity(playerEntity);

        auto pos = entity->Get<Position>();
        pos->mPosition.x += 1.f;

        auto transform = entity->GetTransform();
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.x -= 1;
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.z += 1;
        gEntityManager.GetEntity(floor)->GetTransform().mPosition.y = -1;

        constexpr float floorScale = 10.f;
        gEntityManager.GetEntity(floor)->GetTransform().mScale = nv::float3(floorScale, floorScale, floorScale);
    }

    void TestJob(void* data)
    {
        NV_EVENT("Test Job");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    Handle<jobs::Job> sJobHandle = Null<jobs::Job>();

    void DriverSystem::Update(float deltaTime, float totalTime)
    {
        using namespace input;

        gEntityManager.ForEachEntity([](Entity* entity)
        {
            using namespace graphics;
            using namespace graphics::components;

            // Create bounding box for entities that have renderable component
            // if they don't have one
            if(entity->Has<Renderable>() && !entity->Has<BoundingBox>())
            {
                auto renderable = entity->Get<Renderable>();
                if (renderable->mMesh.IsNull())
                    return;

                auto pMesh = gResourceManager->GetMesh(renderable->mMesh);
                if (pMesh)
                {
                    auto pBox = entity->Add<math::BoundingBox>();
                    *pBox = pMesh->GetBoundingBox();
                }
            }
        });

        if (jobs::IsFinished(sJobHandle))
        {
            sJobHandle = Null<jobs::Job>();
        }

        if (sJobHandle.IsNull())
        {
            sJobHandle = jobs::Execute(TestJob);
        }

        auto pPool = gComponentManager.GetPool<math::BoundingBox>();
        EntityComponents<math::BoundingBox> comps;
        pPool->GetEntityComponents(comps);
        
        for (uint32_t i = 0; i < comps.Size(); ++i)
        {
            auto pEntity = comps[i].mpEntity;
            auto transform = pEntity->GetTransform();
            auto meshHandle = pEntity->Get<graphics::components::Renderable>()->mMesh;
            auto pMesh = graphics::gResourceManager->GetMesh(meshHandle);
            if(!pMesh->HasBones())
                pMesh->GetBoundingBox().mBounding.Transform(comps[i].mpComponent->mBounding, math::Load(transform.GetTransformMatrix()));
            else
            {
                comps[i].mpComponent->mBounding.Center = transform.mPosition;
            }
        }

        if (mFrameRecordState != FRAME_RECORD_REWINDING)
        {
            auto transform = entity->GetTransform();
            transform.mPosition.y = sin(totalTime * 2.f);
        }

        if (input::IsKeyPressed(input::Keys::Escape))
            Instance::SetInstanceState(INSTANCE_STATE_STOPPED);

        if (IsKeyPressed(Keys::OemTilde))
        {
            sbEnableDebugUI = !sbEnableDebugUI;
            sDelayTimer = 0.f;
            graphics::SetEnableDebugUI(sbEnableDebugUI);
        }

        FrameRecordEvent frameEvent;

        if (IsKeyPressed(Keys::F5))
        {
            std::ofstream file("save.bin");
            SerializeScene(file);
        }

        if (IsKeyPressed(Keys::F6))
        {
            std::ifstream file("save.bin");
            DeserializeScene(file);
        }

        static bool bEnableFrameRecord = true;

        if (bEnableFrameRecord)
        {
            if (IsKeyDown(Keys::F))
            {
                mFrameRecordState = FRAME_RECORD_REWINDING;
            }
            else
                mFrameRecordState = FRAME_RECORD_IN_PROGRESS;

            switch (mFrameRecordState)
            {
            case FRAME_RECORD_IN_PROGRESS:
                PushFrame();
                break;
            case FRAME_RECORD_REWINDING:
                if (!PopFrame())
                    mFrameRecordState = FRAME_RECORD_IN_PROGRESS;
                break;
            default:
                break;
            }

            frameEvent.mState = mFrameRecordState;
            gEventBus.Publish(&frameEvent);
        }
    }

    void PushFrame()
    {
        FrameData& frame = gFrameStack.emplace_back();
        const auto& componentPools = gComponentManager.GetAllPools();
        for (auto& pool : componentPools)
        {
            pool.second->SerializeForFrame(frame.mStream);
        }
    }

    bool PopFrame()
    {
        if (gFrameStack.empty())
            return false;

        FrameData& frame = gFrameStack.back();
        const auto& componentPools = gComponentManager.GetAllPools();
        for (auto& pool : componentPools)
        {
            pool.second->DeserializeForFrame(frame.mStream);
        }

        gFrameStack.pop_back();
        return true;
    }

    void DriverSystem::Destroy()
    {
    }
}
