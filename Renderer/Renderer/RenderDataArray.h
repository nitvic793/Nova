#pragma once

#include <Renderer/CommonDefines.h>
#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Lib/ConcurrentQueue.h>
#include <Engine/Transform.h>
#include <Interop/ShaderInteropTypes.h>
#include <atomic>

namespace nv::graphics
{
    class Mesh;
    class ConstantBufferPool;
    struct Material;

    struct RenderDescriptors
    {
        using CBV = ConstantBufferView;
        nv::Vector<CBV>             mObjectCBs;
        nv::Vector<CBV>             mMaterialCBs;
    };

    struct RenderObject
    {
        Mesh*&          mpMesh;
        Material*&      mpMaterial;
        ObjectData&     mObjectData;
    };

    struct RenderData
    {
        size_t      mSize;
        Mesh**      mppMeshes;
        Material**  mppMaterials;
        ObjectData* mpObjectData;

        RenderData():
            mSize(0),
            mppMaterials(nullptr),
            mppMeshes(nullptr),
            mpObjectData(nullptr)
        {}

        RenderData(size_t size)
            : mSize(size)
        {
            Init(size);
        }

        RenderData(RenderData&& p) noexcept :
            mSize(p.mSize),
            mppMeshes(p.mppMeshes),
            mppMaterials(p.mppMaterials),
            mpObjectData(p.mpObjectData)
        {
            p.mSize = 0;
            p.mppMaterials = nullptr;
            p.mppMeshes = nullptr;
            p.mpObjectData = nullptr;
        }

        RenderData& operator=(const RenderData& rhs) = delete;

        RenderData& operator=(RenderData&& rhs) noexcept
        {
            Clear();
            mSize = rhs.mSize;
            mppMeshes = rhs.mppMeshes;
            mppMaterials = rhs.mppMaterials;
            mpObjectData = rhs.mpObjectData;

            rhs.mSize = 0;
            rhs.mppMaterials = nullptr;
            rhs.mppMeshes = nullptr;
            rhs.mpObjectData = nullptr;
            return *this;
        }

        RenderObject operator[](size_t idx) const
        {
            return RenderObject{ .mpMesh = mppMeshes[idx], .mpMaterial = mppMaterials[idx], .mObjectData = mpObjectData[idx] };
        }

        RenderObject begin() const
        {
            return RenderObject{ .mpMesh = mppMeshes[0], .mpMaterial = mppMaterials[0], .mObjectData = mpObjectData[0] };
        }

        RenderObject end() const
        {
            return RenderObject{ .mpMesh = mppMeshes[mSize - 1], .mpMaterial = mppMaterials[mSize - 1], .mObjectData = mpObjectData[mSize - 1] };
        }

        void Init(size_t size)
        {
            mSize = size;
            mppMeshes = (Mesh**)Alloc(sizeof(Mesh*) * size);
            mppMaterials = (Material**)Alloc(sizeof(Material*) * size);
            mpObjectData = (ObjectData*)Alloc(sizeof(ObjectData) * size);
        }

        void Clear()
        {
            if (mSize > 0)
            {
                assert(mppMaterials);
                assert(mppMeshes);
                assert(mpObjectData);

                Free(mppMeshes);
                Free(mppMaterials);
                Free(mpObjectData);
                mSize = 0;
                mppMeshes = nullptr;
                mppMaterials = nullptr;
                mpObjectData = nullptr;
            }
        }

        ~RenderData()
        {
            Clear();
        }
    };

    struct RenderDataArray
    {
        using CBV = ConstantBufferView;
        static constexpr uint32_t MAX_RENDER_BUFFER = 2;

        RenderDescriptors               mRenderDescriptors;
        ConcurrentQueue<RenderData>     mRenderDataQueue;
        uint32_t                        mRenderThreadId;

        RenderDataArray();
        void Clear(); // Switch buffer and clears non current buffer

        void QueueRenderData();
        bool PopRenderData(RenderData& out) { if (mRenderDataQueue.IsEmpty()) return false; mRenderDataQueue.Pop(out); return true; }
        void GenerateDescriptors(RenderData& rd);

        // Get data from "current" buffer
        constexpr Span<CBV>             GetObjectDescriptors()      const { return mRenderDescriptors.mObjectCBs.Span();}
        constexpr Span<CBV>             GetMaterialDescriptors()    const { return mRenderDescriptors.mMaterialCBs.Span(); }
    };

    extern ConstantBufferPool* gpConstantBufferPool;
}