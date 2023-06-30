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

namespace nv
{
    using namespace ecs;
    using namespace input;

    struct PlayerComponent : public ecs::IComponent
    {
        float mSpeed = 10.f;
    };

    Entity* gpPlayerEntity = nullptr;
    PlayerComponent* gpPlayerComponent = nullptr;


    void PlayerController::Init()
    {
        mPlayerEntity = CreateEntity(ID("Mesh/cube.obj"), ID("Bronze"));
        gpPlayerEntity = gEntityManager.GetEntity(mPlayerEntity);
        gpPlayerComponent = gpPlayerEntity->Add<PlayerComponent>();
    }

    void PlayerController::Update(float deltaTime, float totalTime)
    {
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

    ecs::Entity* GetPlayerEntity()
    {
        return gpPlayerEntity;
    }
}