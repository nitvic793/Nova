#include "pch.h"
#include "CameraSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Input/Input.h>
#include <Engine/Log.h>
#include <Interop/ShaderInteropTypes.h>
#include <Engine/Camera.h>
#include <Renderer/Window.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include "EntityCommon.h"
#include <DebugUI/DebugUIPass.h>

#include "Player.h"

namespace nv
{
    using namespace ecs;
    using namespace math;
    using namespace graphics;

    Handle<Entity> CreateCamera(float3 position, const char* pDebugName = nullptr)
    {
        auto entityHandle = CreateEntity(RES_ID_NULL, RES_ID_NULL, pDebugName);
        auto cameraEntity = gEntityManager.GetEntity(entityHandle);
        auto comp = cameraEntity->Add<CameraComponent>();
        cameraEntity->GetTransform().mPosition = { 0, 0, -15 };
        comp->mCamera.SetParams(CameraDesc{ .mWidth = (float)graphics::gWindow->GetWidth(), .mHeight = (float)gWindow->GetHeight() });
        comp->mCamera.UpdateViewProjection();
        return entityHandle;
    }

    void CameraSystem::Init()
    {
        mEditorCamera = CreateCamera({ 0, 0, -15 }, "Editor Camera");
        mPlayerCamera = CreateCamera({ 0, 0, -15 }, "Player Camera");
        mPrevPos = { (float)input::GetInputState().mMouse.GetLastState().x,  (float)input::GetInputState().mMouse.GetLastState().y };
        graphics::SetActiveCamera(mEditorCamera);
    }

    void EditorCameraUpdate(float deltaTime, CameraComponent* component, Handle<Entity> cameraEntity, math::float2& prevPos)
    {
        const bool isDebugUIActive = graphics::IsDebugUIInputActive();

        float speed = 3.f;
        float speedMultiplier = 3.f;
        float mouseSpeed = 0.005f;

        auto transform = gEntityManager.GetEntity(cameraEntity)->GetTransform();
        Camera& camera = component->mCamera;
        camera.SetPreviousViewProjection();

        auto up = VectorSet(0, 1, 0, 0);
        auto dir = Load(camera.GetDirection());
        auto pos = Load(transform.mPosition);
        auto rot = Load(transform.mRotation);

        auto rotation = VectorSet(0, 0, 0, 0);
        float angle = 0.f;
        QuaternionToAxisAngle(rotation, angle, rot);

        using namespace input;

        if (IsKeyDown(Keys::LeftShift))
            speed = speed * speedMultiplier;

        if (IsKeyDown(Keys::A))
        {
            auto leftDir = Vector3Cross(dir, up);
            pos = pos + leftDir * deltaTime * speed;
        }

        if (IsKeyDown(Keys::D))
        {
            auto rightDir = Vector3Cross(-dir, up);
            pos = pos + rightDir * deltaTime * speed;
        }

        if (IsKeyDown(Keys::W))
        {
            pos = pos + dir * deltaTime * speed;
        }

        if (IsKeyDown(Keys::S))
        {
            pos = pos - dir * deltaTime * speed;
        }

        float xDiff = 0;
        float yDiff = 0;

        auto mousePos = float2{ (float)input::GetInputState().mMouse.GetLastState().x,  (float)input::GetInputState().mMouse.GetLastState().y };;
        if (input::LeftMouseButtonState() == ButtonState::HELD && !isDebugUIActive)
        {
            xDiff = (float)(mousePos.x - prevPos.x) * mouseSpeed;
            yDiff = (float)(mousePos.y - prevPos.y) * mouseSpeed;
        }

        auto camRotatin = camera.GetRotation();
        camRotatin.x += yDiff;
        camRotatin.y += xDiff;

        camera.SetRotation(camRotatin);

        if(!isDebugUIActive)
            Store(pos, transform.mPosition);

        camera.SetPosition(transform.mPosition);
        camera.UpdateViewProjection();
    }

    void PlayerCameraUpdate(Handle<Entity> camHandle, float deltaTime, float totalTime)
    {
        Entity* player = GetPlayerEntity();
        auto transform = player->GetTransform();
        
        Entity* camera = gEntityManager.GetEntity(camHandle);
        auto camTransform = camera->GetTransform();
        auto& camComponent = camera->Get<CameraComponent>()->mCamera;
        camComponent.SetPreviousViewProjection();
        camTransform.mPosition.x = transform.mPosition.x + 5;
        camTransform.mPosition.y = transform.mPosition.z;
        camTransform.mPosition.z = transform.mPosition.z - 15;

        camComponent.SetPosition(camTransform.mPosition);
        camComponent.UpdateViewProjection();
    }

    void CameraSystem::Update(float deltaTime, float totalTime)
    {
#if NV_ENABLE_DEBUG_UI
        const bool isDebugUIEnabled = graphics::IsDebugUIEnabled();
#else
        constexpr bool isDebugUIEnabled = false;
#endif
        if (isDebugUIEnabled)
        {
            graphics::SetActiveCamera(mEditorCamera);
            auto comp = gEntityManager.GetEntity(mEditorCamera)->Get<CameraComponent>();
            EditorCameraUpdate(deltaTime, comp, mEditorCamera, mPrevPos);
            mPrevPos = { (float)input::GetInputState().mMouse.GetLastState().x,  (float)input::GetInputState().mMouse.GetLastState().y };
        }
        else
        {
            graphics::SetActiveCamera(mPlayerCamera);
            PlayerCameraUpdate(mPlayerCamera, deltaTime, totalTime);
        }
    }

    void CameraSystem::Destroy()
    {
    }
}

