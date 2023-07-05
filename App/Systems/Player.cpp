#include "pch.h"
#include "Player.h"

#include "EntityCommon.h"
#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Input/Input.h>
#include <Engine/Log.h>
#include <Engine/Instance.h>
#include <Interop/ShaderInteropTypes.h>
#include <Input/InputState.h>
#include <Engine/EventSystem.h>

namespace nv
{
    using namespace ecs;
    using namespace input;

    struct PlayerComponent : public ecs::IComponent
    {
        float mSpeed = 10.f;
    };

    Handle<Entity> gpPlayerEntity = Null<Entity>();
    PlayerComponent* gpPlayerComponent = nullptr;
    FrameRecordState gFrameState = FRAME_RECORD_STOPPED;

    void PlayerController::Init()
    {
        mPlayerEntity = CreateEntity(ID("Mesh/cone.obj"), ID("Bronze"), "Player");
        gpPlayerEntity = mPlayerEntity;
        auto playerEntity = gEntityManager.GetEntity(mPlayerEntity);
        gpPlayerComponent = playerEntity->Add<PlayerComponent>();

        gEventBus.Subscribe(this, &PlayerController::OnFrameRecordStateChange);
    }

    void PlayerController::Update(float deltaTime, float totalTime)
    {

        // TODO:
        // 1. Scene Editor + ImGui +  Scene Serialization/Deserialization
        // 2. Basic Level with Enemies?
        // 3. PBR + IBL 
        // 4. Animation

        if (gFrameState == FRAME_RECORD_REWINDING)
            return;

        Entity* entity = gEntityManager.GetEntity(mPlayerEntity);

        auto transform = entity->GetTransform();
        const float speed = gpPlayerComponent->mSpeed;

        if (IsKeyDown(Keys::D))
        {
            transform.mPosition.x += speed * deltaTime;
        }

        if (IsKeyDown(Keys::A))
        {
            transform.mPosition.x -= speed * deltaTime;
        }
    }

    void PlayerController::Destroy()
    {
    }

    void PlayerController::OnFrameRecordStateChange(FrameRecordEvent* pEvent)
    {
        gFrameState = pEvent->mState;
    }

    ecs::Entity* GetPlayerEntity()
    {
        return gEntityManager.GetEntity(gpPlayerEntity);
    }
}