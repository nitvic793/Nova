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
    using namespace graphics::components;

    enum PlayerState : uint32_t
    {
        PLAYER_STATE_IDLE = 0,
        PLAYER_STATE_RUNNING = 1,
        PLAYER_STATE_JUMP = 2
    };

    struct PlayerComponent : public ecs::IComponent
    {
        float       mSpeed          = 10.f;
        PlayerState mPlayerState    = PLAYER_STATE_IDLE;
        float3      mVelocity       = float3(0,0,0);

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(mSpeed);
            archive(mPlayerState);
        }
    };

    Handle<Entity> gpPlayerEntity = Null<Entity>();
    PlayerComponent* gpPlayerComponent = nullptr;
    FrameRecordState gFrameState = FRAME_RECORD_STOPPED;

    constexpr float DegToRags(float x)
    {
        return 2 * XM_PI * x * (1.f / 360.f);
    }

    void PlayerController::Init()
    {
        mPlayerEntity = CreateEntity(ID("Mesh/male.fbx"), ID("Bronze"), "Player");
        gpPlayerEntity = mPlayerEntity;
        auto playerEntity = gEntityManager.GetEntity(mPlayerEntity);
        gpPlayerComponent = playerEntity->Add<PlayerComponent>();
        auto transform = playerEntity->GetTransform();
        transform.mScale = float3(0.01f, 0.01f, 0.01f);

        auto rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRags(270));
        math::Store(rotation, transform.mRotation);

        playerEntity->Get<AnimationComponent>()->mCurrentAnimationIndex = 1;
        playerEntity->Get<AnimationComponent>()->mIsPlaying = true;
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
        auto anim = entity->Get<AnimationComponent>();

        auto transform = entity->GetTransform();
        const float speed = gpPlayerComponent->mSpeed;

        // TODO: Use Velocity instead of manipulating speed directly. 
        // Apply terms to velocity and then position = position + velocity * time;
        gpPlayerComponent->mPlayerState = PLAYER_STATE_IDLE;

        if (transform.mPosition.y > 0)
        {
            transform.mPosition.y -= 2.f * deltaTime;
            gpPlayerComponent->mPlayerState = PLAYER_STATE_JUMP;
        }

        if (IsKeyDown(Keys::Space))
        {
            gpPlayerComponent->mPlayerState = PLAYER_STATE_JUMP;
        }

        if (IsKeyDown(Keys::D))
        {
            auto rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRags(-90));
            math::Store(rotation, transform.mRotation);

            transform.mPosition.x += speed * deltaTime;
            gpPlayerComponent->mPlayerState = PLAYER_STATE_RUNNING;
        }

        if (IsKeyDown(Keys::A))
        {
            auto rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRags(90));
            math::Store(rotation, transform.mRotation);

            transform.mPosition.x -= speed * deltaTime;
            gpPlayerComponent->mPlayerState = PLAYER_STATE_RUNNING;
        }

        switch (gpPlayerComponent->mPlayerState)
        {
        case PLAYER_STATE_IDLE:
            anim->mCurrentAnimationIndex = 1;
            break;
        case PLAYER_STATE_JUMP:
            anim->mCurrentAnimationIndex = 2;
            break;
        case PLAYER_STATE_RUNNING:
            anim->mCurrentAnimationIndex = 3;
            break;
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