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

    const bool AreNearEqual(float a, float b)
    {
        return fabs(a - b) < FLT_EPSILON;
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
        const bool bIsDebugUIEnabled = IsDebugUIEnabled();
        if (bIsDebugUIEnabled)
            return;

        using namespace math;
        const float VELOCITY_DECAY_SPEED = 15.f;
        const float JUMP_MULTIPLIER = 2.f;
        // TODO:
        // 1. [Done] Scene Editor + ImGui +  Scene Serialization/Deserialization
        // 2. Basic Level with Enemies?
        // 3. PBR + IBL 
        // 4. [Done] Animation

        if (gFrameState == FRAME_RECORD_REWINDING)
            return;

        auto zero = VectorSet(0, 0, 0, 0);
        auto right = math::VectorSet(1, 0, 0, 0);
        auto left = math::VectorSet(-1, 0, 0, 0);
        auto up = math::VectorSet(0, 1, 0, 0);
        auto epsilon = VectorSet(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON, FLT_EPSILON);

        Entity* entity = gEntityManager.GetEntity(mPlayerEntity);
        auto anim = entity->Get<AnimationComponent>();

        auto transform = entity->GetTransform();
        const float speed = gpPlayerComponent->mSpeed;
        auto velocity = Load(gpPlayerComponent->mVelocity);
        auto maxVelocity = VectorSet(speed, speed * JUMP_MULTIPLIER, speed, speed);
        auto currentPos = Load(transform.mPosition);


        velocity = XMVectorLerp(velocity, zero, deltaTime * VELOCITY_DECAY_SPEED);
        if(XMVector3NearEqual(velocity, zero, epsilon))
            velocity = zero;

        // TODO: Use Velocity instead of manipulating speed directly. 
        // Apply terms to velocity and then position = position + velocity * time;
        PlayerState state = PLAYER_STATE_IDLE;
        PlayerState prevState = gpPlayerComponent->mPlayerState;

        if (IsKeyDown(Keys::Space) && AreNearEqual(transform.mPosition.y, 0.f))
        {
            velocity = velocity + up * speed * JUMP_MULTIPLIER;
        }

        if (IsKeyDown(Keys::D))
        {
            auto rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRags(-90));
            math::Store(rotation, transform.mRotation);

            velocity = velocity + right * speed;
        }

        if (IsKeyDown(Keys::A))
        {
            auto rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRags(90));
            math::Store(rotation, transform.mRotation);

            velocity = velocity + left * speed;
        }

        velocity = XMVectorMin(maxVelocity, velocity);
        velocity = XMVectorMax(-maxVelocity, velocity);
        currentPos = currentPos + velocity * deltaTime;

        Store(currentPos, transform.mPosition);
        Store(velocity, gpPlayerComponent->mVelocity);

        float lDot = XMVectorGetX(Vector3Dot(velocity, left));
        float rDot = XMVectorGetX(Vector3Dot(velocity, right));
        float uDot = XMVectorGetX(Vector3Dot(velocity, up));

        if (lDot > 0.1f || rDot > 0.1f)
        {
            state = PLAYER_STATE_RUNNING;
        }

        if (transform.mPosition.y > FLT_EPSILON)
        {
            state = PLAYER_STATE_JUMP;
        }

        gpPlayerComponent->mPlayerState = state;

        anim->mAnimationSpeed = 1.f;
        switch (gpPlayerComponent->mPlayerState)
        {
        case PLAYER_STATE_IDLE:
            anim->mCurrentAnimationIndex = 1;
            gpPlayerComponent->mVelocity.y = 0.f;
            break;
        case PLAYER_STATE_JUMP:
            anim->mAnimationSpeed = 0.8f;
            anim->mCurrentAnimationIndex = 2;
            transform.mPosition.y -= 3.f * deltaTime;
            transform.mPosition.y = std::max(0.f, transform.mPosition.y);
            break;
        case PLAYER_STATE_RUNNING:
            anim->mCurrentAnimationIndex = 3;
            gpPlayerComponent->mVelocity.y = 0.f;
            break;
        }

        if(state != prevState)
            anim->mTotalTime = 0.f;
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