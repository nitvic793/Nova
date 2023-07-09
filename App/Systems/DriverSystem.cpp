#include "pch.h"
#include "DriverSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Engine/Component.h>
#include <Engine/EventSystem.h>

#include <Input/Input.h>
#include <Engine/Log.h>
#include <Engine/Instance.h>
#include <Interop/ShaderInteropTypes.h>
#include <DebugUI/DebugUIPass.h>
#include "EntityCommon.h"
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

        setupSky();

        entity = gEntityManager.GetEntity(playerEntity);

        auto pos = entity->Get<Position>();
        pos->mPosition.x += 1.f;

        auto transform = entity->GetTransform();
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.x -= 1;
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.z += 1;
    }

    void DriverSystem::Update(float deltaTime, float totalTime)
    {
        using namespace input;

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

        static bool bEnableFrameRecord = false;

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
