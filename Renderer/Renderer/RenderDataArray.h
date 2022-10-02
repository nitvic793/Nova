#pragma once

#include <Renderer/CommonDefines.h>
#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Engine/Transform.h>
#include <Interop/ShaderInteropTypes.h>
#include <atomic>

namespace nv::graphics
{
    class Mesh;
    class ConstantBufferPool;
    struct Material;

    struct RenderBuffer
    {
        using CBV = ConstantBufferView;
        nv::Vector<ObjectData>      mData;
        nv::Vector<CBV>             mObjectCBs;
        nv::Vector<CBV>             mMaterialCBs;
        nv::Vector<Material*>       mMaterials;
        nv::Vector<Mesh*>           mMeshes;
    };

    struct RenderDataArray
    {
        using CBV = ConstantBufferView;
        static constexpr uint32_t MAX_RENDER_BUFFER = 2;

        RenderBuffer            mRenderBuffer[MAX_RENDER_BUFFER];
        std::atomic<uint32_t>   mCurrentBuffer;
        std::atomic<uint32_t>   mProduced;
        std::atomic<uint32_t>   mConsumed;

        RenderDataArray();
        void Clear(); // Switch buffer and clears non current buffer
        void Insert(Mesh* mesh, Material* material, const TransformRef& transform); // Insert into non-current buffer.

        void                  IncrementProduced() { mProduced.store(mProduced.load() + 1); }
        void                  IncrementConsumed() { mConsumed.store(mConsumed.load() + 1); }

        uint32_t              GetConsumedCount() const { return mConsumed.load(); }
        uint32_t              GetProducedCount() const { return mProduced.load(); }

        // Get data from "current" buffer
        constexpr Span<ObjectData>      GetObjectData()     const { return mRenderBuffer[mCurrentBuffer.load()].mData.Span(); }
        constexpr Span<CBV>             GetObjectCBs()      const { return mRenderBuffer[mCurrentBuffer.load()].mObjectCBs.Span();}
        constexpr Span<CBV>             GetMaterialCBs()    const { return mRenderBuffer[mCurrentBuffer.load()].mMaterialCBs.Span(); }
        constexpr Span<Material*>       GetMaterials()      const { return mRenderBuffer[mCurrentBuffer.load()].mMaterials.Span(); }
        constexpr Span<Mesh*>           GetMeshes()         const { return mRenderBuffer[mCurrentBuffer.load()].mMeshes.Span(); }
    };

    extern ConstantBufferPool* gpConstantBufferPool;
}