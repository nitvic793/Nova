#include "pch.h"
#include "DriverSystem.h"

#include <Renderer/ResourceManager.h>
#include <Components/Material.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Input/Input.h>
#include <Engine/Log.h>

namespace nv
{
    ecs::Entity* entity;
    void DriverSystem::Init()
    {
        using namespace ecs;
        using namespace graphics;

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
            return entity;
        };

        Entity* entity1 = createEntity(ID("Mesh/torus.obj"), ID("Floor"));
        Entity* entity2 = createEntity(ID("Mesh/cube.obj"), ID("Bronze"));
        entity = entity1;

        auto pos = entity1->Get<Position>();
        pos->mPosition.x += 1.f;

        auto transform = entity1->GetTransform();
        entity2->GetTransform().mPosition.x -= 1;
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
