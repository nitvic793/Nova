#include "pch.h"
#include "RenderDataArray.h"

#include <Renderer/ResourceManager.h>
#include <Renderer/ConstantBufferPool.h>
#include <Renderer/Device.h>
#include <Engine/EntityComponent.h>
#include <Components/Renderable.h>
#include <Components/Material.h>

namespace nv::graphics
{
    ConstantBufferPool* gpConstantBufferPool = nullptr;
    constexpr size_t MAX_OBJECT_COUNT = 1024;
    constexpr size_t MAX_OBJECT_DESCRIPTOR_COUNT = MAX_OBJECT_COUNT;

    RenderDataArray::RenderDataArray()
    {
    }

    void RenderDataArray::Clear()
    {
        mRenderDescriptors.mObjectCBs.Clear();
        mRenderDescriptors.mMaterialCBs.Clear();
    }

    void RenderDataArray::QueueRenderData()
    {
        constexpr size_t MAX_QUEUED_RENDER_DATA = 3;
        if (mRenderDataQueue.Size() > MAX_QUEUED_RENDER_DATA)
        {
            // Ensure data in queue is fresh by dequeing old data if we're at threshold.
            // TODO: Test with more objects.
            RenderData renderData;
            mRenderDataQueue.Pop(renderData); 
        }

        auto renderables = ecs::gComponentManager.GetComponents<components::Renderable>();

        if (renderables.Size() > 0)
        {
            RenderData renderData(renderables.Size());

            auto positions = ecs::gComponentManager.GetComponents<Position>();
            auto scales = ecs::gComponentManager.GetComponents<Scale>();
            auto rotations = ecs::gComponentManager.GetComponents<Rotation>();

            for (size_t i = 0; i < renderables.Size(); ++i)
            {
                auto& renderable = renderables[i];
                auto mesh = gResourceManager->GetMesh(renderable.mMesh);
                auto mat = gResourceManager->GetMaterial(renderable.mMaterial);

                auto pos = &positions[i];
                auto scale = &scales[i];
                auto rotation = &rotations[i];
                TransformRef transform = { pos->mPosition, rotation->mRotation, scale->mScale };

                auto rd = renderData[i];
                rd.mObjectData.World = transform.GetTransformMatrixTransposed();
                rd.mpMesh = mesh;
                rd.mpMaterial = mat;
            }

            mRenderDataQueue.Push(renderData);
        }
    }

    void RenderDataArray::GenerateDescriptors(RenderData& rd)
    {
        Clear();
        for (uint32_t i = 0; i < rd.mSize; ++i)
        {
            auto objectCb = gpConstantBufferPool->GetConstantBuffer<ObjectData, MAX_OBJECT_DESCRIPTOR_COUNT>();
            auto matCb = gpConstantBufferPool->GetConstantBuffer<MaterialData, MAX_OBJECT_DESCRIPTOR_COUNT>();
            mRenderDescriptors.mObjectCBs.Push(objectCb);
            mRenderDescriptors.mMaterialCBs.Push(matCb);
        }
    }
}


