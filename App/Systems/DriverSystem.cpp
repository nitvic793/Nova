#include "pch.h"
#include "DriverSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Input/Input.h>
#include <Engine/Log.h>
#include <Interop/ShaderInteropTypes.h>

namespace nv
{
    ecs::Entity* entity;

    void DriverSystem::Init()
    {
        using namespace ecs;
        using namespace graphics;
        using namespace components;

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

        auto entity1 = createEntity(ID("Mesh/torus.obj"), ID("Floor"));
        auto entity2 = createEntity(ID("Mesh/cube.obj"), ID("Bronze"));
        auto e3 = createEntity(RES_ID_NULL, RES_ID_NULL);
        auto directionalLight = gEntityManager.GetEntity(e3)->Add<DirectionalLight>();
        directionalLight->Color = float3(0.9f, 0.9f, 0.9f);
        directionalLight->Intensity = 1.f;

        auto skybox = gEntityManager.GetEntity(e3)->Add<SkyboxComponent>();
        skybox->mSkybox = gResourceManager->GetTextureHandle(ID("Textures/SunnyCubeMap.dds"));

        Store(Vector3Normalize(VectorSet(1, -1, 1, 0)), directionalLight->Direction);

        entity = gEntityManager.GetEntity(entity2);

        auto pos = entity->Get<Position>();
        pos->mPosition.x += 1.f;

        auto transform = entity->GetTransform();
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.x -= 1;
        gEntityManager.GetEntity(entity1)->GetTransform().mPosition.z += 1;
    }

    void DriverSystem::Update(float deltaTime, float totalTime)
    {
        auto transform = entity->GetTransform();
        transform.mPosition.y = sin(totalTime * 2.f);

        auto kb = input::GetInputState().mpKeyboardInstance->GetState();

        if (input::IsKeyComboPressed(input::Keys::LeftShift, input::Keys::T))
        {
            log::Info("LShift + T Pressed");
        }

        if (input::IsKeyPressed(input::Keys::F) && input::IsKeyDown(input::Keys::LeftShift))
            log::Info("LShift + F Pressed");

        if (input::LeftMouseButtonState() == input::ButtonState::PRESSED)
            log::Info("Left mouse button pressed");
    }

    void DriverSystem::Destroy()
    {
    }
}
