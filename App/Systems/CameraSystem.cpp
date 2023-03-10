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

namespace nv
{
    using namespace ecs;
    using namespace math;
    using namespace graphics;

    void CameraSystem::Init()
    {
        const auto createEntity = [&](ResID mesh, ResID mat, const Transform& transform = Transform())
        {
            Handle<Material> matHandle = gResourceManager->GetMaterialHandle(mat);
            Handle<Mesh> meshHandle = gResourceManager->GetMeshHandle(mesh);

            Handle<Entity> e = ecs::gEntityManager.Create();
            Entity* entity = gEntityManager.GetEntity(e);

            entity->AttachTransform(transform);
            auto renderable = entity->Add<components::Renderable>();
            renderable->mMaterial = matHandle;
            renderable->mMesh = meshHandle;
            return e;
        };

        mEditorCamera = createEntity(RES_ID_NULL, RES_ID_NULL);
        auto comp = gEntityManager.GetEntity(mEditorCamera)->Add<CameraComponent>();
        gEntityManager.GetEntity(mEditorCamera)->GetTransform().mPosition = { 0, 0, -15 };
        comp->mCamera.SetParams(CameraDesc{ .mWidth = (float)graphics::gWindow->GetWidth(), .mHeight = (float)gWindow->GetHeight() });
        comp->mCamera.UpdateViewProjection();
        mPrevPos = { (float)input::GetInputState().mMouse.GetLastState().x,  (float)input::GetInputState().mMouse.GetLastState().y };
    }

    void CameraSystem::Update(float deltaTime, float totalTime)
    {
#if NV_ENABLE_DEBUG_UI
        const bool isDebugUIActive = graphics::IsDebugUIInputActive();
#else
        constexpr bool isDebugUIActive = false;
#endif

        float speed = 3.f;
        float speedMultiplier = 3.f;
        float mouseSpeed = 0.005f;

        auto comp = gEntityManager.GetEntity(mEditorCamera)->Get<CameraComponent>();
        auto transform = gEntityManager.GetEntity(mEditorCamera)->GetTransform();
        Camera& camera = comp->mCamera;

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
            xDiff = (float)(mousePos.x - mPrevPos.x) * mouseSpeed;
            yDiff = (float)(mousePos.y - mPrevPos.y) * mouseSpeed;
        }

        auto camRotatin = camera.GetRotation();
        camRotatin.x += yDiff;
        camRotatin.y += xDiff;

        camera.SetRotation(camRotatin);

        if(!isDebugUIActive)
            Store(pos, transform.mPosition);

        mPrevPos = { (float)input::GetInputState().mMouse.GetLastState().x,  (float)input::GetInputState().mMouse.GetLastState().y };

        camera.SetPosition(transform.mPosition);
        camera.UpdateViewProjection();
    }

    void CameraSystem::Destroy()
    {
    }
}

