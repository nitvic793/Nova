#include "pch.h"
#include "RenderDataArray.h"

#include <Renderer/ResourceManager.h>
#include <Renderer/ConstantBufferPool.h>
#include <Renderer/Device.h>

namespace nv::graphics
{
    ConstantBufferPool* gpConstantBufferPool = nullptr;

    RenderDataArray::RenderDataArray():
        mCurrentBuffer(1),
        mProduced(0),
        mConsumed(0)
    {
    }

    void RenderDataArray::Clear()
    {
        // Switch buffer and clear non-current buffer. 
        const uint32_t idx = mCurrentBuffer.load();
        mCurrentBuffer.store((mCurrentBuffer.load() + 1) % MAX_RENDER_BUFFER);
        mRenderBuffer[idx].mData.Clear();
        mRenderBuffer[idx].mObjectCBs.Clear();
        mRenderBuffer[idx].mMaterialCBs.Clear();
        mRenderBuffer[idx].mMaterials.Clear();
        mRenderBuffer[idx].mMeshes.Clear();
    }

    void RenderDataArray::Insert(Mesh* mesh, Material* material, const TransformRef& transform)
    {
        constexpr size_t MAX_OBJECT_COUNT = 1024;
        constexpr size_t MAX_OBJECT_DESCRIPTOR_COUNT = MAX_OBJECT_COUNT;

        assert(mRenderBuffer[mCurrentBuffer].mData.Size() <= MAX_OBJECT_COUNT); // Currently only supports 1k objects.

        auto objectCb = gpConstantBufferPool->GetConstantBuffer<ObjectData, MAX_OBJECT_DESCRIPTOR_COUNT>();
        auto matCb = gpConstantBufferPool->GetConstantBuffer<MaterialData, MAX_OBJECT_DESCRIPTOR_COUNT>();
        ObjectData objData = { transform.GetTransformMatrixTransposed() };

        {
            // Insert into non current buffer
            const uint32_t idx = (mCurrentBuffer.load() + 1) % MAX_RENDER_BUFFER; 
            mRenderBuffer[idx].mData.Push(objData);
            mRenderBuffer[idx].mObjectCBs.Push(objectCb);
            mRenderBuffer[idx].mMaterialCBs.Push(matCb);
            mRenderBuffer[idx].mMaterials.Push(material);
            mRenderBuffer[idx].mMeshes.Push(mesh);
        }
    }
}


