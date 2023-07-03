#include "pch.h"
#include "DriverSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Input/Input.h>
#include <Engine/Log.h>
#include <Engine/Instance.h>
#include <Interop/ShaderInteropTypes.h>
#include <DebugUI/DebugUIPass.h>
#include "EntityCommon.h"

namespace nv
{
    constexpr float INPUT_DELAY_DEBUG_PRESS = 1.f;
    static bool sbEnableDebugUI = false;

    static float sDelayTimer = 0.f;

    ecs::Entity* entity;

    void DriverSystem::Init()
    {
        using namespace ecs;
        using namespace graphics;
        using namespace components;

        const auto setupSky = [&]()
        {
            auto e3 = CreateEntity(RES_ID_NULL, RES_ID_NULL);
            auto directionalLight = gEntityManager.GetEntity(e3)->Add<DirectionalLight>();
            directionalLight->Color = float3(0.9f, 0.9f, 0.9f);
            directionalLight->Intensity = 1.f;
            auto skybox = gEntityManager.GetEntity(e3)->Add<SkyboxComponent>();
            skybox->mSkybox = gResourceManager->GetTextureHandle(ID("Textures/SunnyCubeMap.dds"));
            Store(Vector3Normalize(VectorSet(1, -1, 1, 0)), directionalLight->Direction);
        };

        auto entity1 = CreateEntity(ID("Mesh/torus.obj"), ID("Floor"));
        auto playerEntity = CreateEntity(ID("Mesh/cube.obj"), ID("Bronze"));

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
        auto transform = entity->GetTransform();
        transform.mPosition.y = sin(totalTime * 2.f);

        auto kb = input::GetInputState().mpKeyboardInstance->GetState();

        if (input::IsKeyPressed(input::Keys::Escape))
            Instance::SetInstanceState(INSTANCE_STATE_STOPPED);

        if (input::IsKeyComboPressed(input::Keys::LeftShift, input::Keys::T))
        {
            log::Info("LShift + T Pressed");
        }

        if (input::IsKeyPressed(input::Keys::F) && input::IsKeyDown(input::Keys::LeftShift))
            log::Info("LShift + F Pressed");

        if (input::LeftMouseButtonState() == input::ButtonState::PRESSED)
            log::Info("Left mouse button pressed");

        if (IsKeyPressed(Keys::OemTilde))
        {
            sbEnableDebugUI = !sbEnableDebugUI;
            sDelayTimer = 0.f;
            graphics::SetEnableDebugUI(sbEnableDebugUI);
        }
    }

    void DriverSystem::Destroy()
    {
    }
}
