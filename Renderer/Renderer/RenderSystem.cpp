#include "pch.h"
#include "RenderSystem.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Engine/JobSystem.h>
#include <Engine/Instance.h>
#include <Engine/Log.h>
#include <Engine/Transform.h>
#include <Engine/EventSystem.h>
#include <Engine/EntityComponent.h>
#include <Debug/Profiler.h>

#include <AssetReload.h>
#include <AssetManager.h>
#include <Types/MeshAsset.h>
#include <Types/ShaderAsset.h>
#include <Types/TextureAsset.h>
#include <Configs/MaterialDatabase.h>

#include <Renderer/ResourceManager.h>
#include <Renderer/Context.h>
#include <Renderer/PipelineState.h>
#include <Renderer/Mesh.h>
#include <Renderer/Window.h>
#include <Renderer/GPUProfile.h>
#include <Renderer/ConstantBufferPool.h>
#include <Components/Material.h>
#include <Components/Renderable.h>

#include <thread>
#include <functional>

namespace nv::graphics
{
    class RenderReloadManager
    {
    public:
        RenderReloadManager(RenderSystem* renderSystem) :
            mRenderSystem(renderSystem)
        {}

        void RegisterPSO(const PipelineStateDesc& desc, Handle<PipelineState>* pso)
        {
            auto psId = gResourceManager->GetShader(desc.mPS)->GetDesc().mShader;
            auto vsId = gResourceManager->GetShader(desc.mVS)->GetDesc().mShader;

            mPsoShaderMap[psId.mId] = pso;
            mPsoShaderMap[vsId.mId] = pso;
        }

        void OnReload(asset::AssetReloadEvent* event)
        {
            if (event->mAssetId.mType == asset::ASSET_SHADER)
            {
                auto pso = mPsoShaderMap.at(event->mAssetId.mId);
                mRenderSystem->QueueReload(pso);
            }
        }

    private:
        HashMap<uint64_t, Handle<PipelineState>*> mPsoShaderMap;
        RenderSystem* mRenderSystem;
    };

    RenderSystem::RenderSystem(uint32_t width, uint32_t height) :
        mCamera(CameraDesc{ .mWidth = (float)width, .mHeight = (float)height }),
        mRect(),
        mViewport(),
        mpConstantBufferPool(nullptr)
    {
        mReloadManager = Alloc<RenderReloadManager>(SystemAllocator::gPtr, this);
        gEventBus.Subscribe(mReloadManager, &RenderReloadManager::OnReload);
    }

    void RenderSystem::Init()
    {
        mpConstantBufferPool = Alloc<ConstantBufferPool>();
        gpConstantBufferPool = mpConstantBufferPool;

        mViewport.mTopLeftX = 0;
        mViewport.mTopLeftY = 0;
        mViewport.mWidth = (float)gWindow->GetWidth();
        mViewport.mHeight = (float)gWindow->GetHeight();
        mViewport.mMinDepth = 0.0f;
        mViewport.mMaxDepth = 1.0f;

        mRect.mLeft = 0;
        mRect.mTop = 0;
        mRect.mRight = gWindow->GetWidth();
        mRect.mBottom = gWindow->GetHeight();

        mFrameCB = mpConstantBufferPool->GetConstantBuffer<FrameData>();
        mObjectDrawData.mObjectCBView = mpConstantBufferPool->GetConstantBuffer<ObjectData>();
        mObjectDrawData.mMaterialCBView = mpConstantBufferPool->GetConstantBuffer<MaterialData>();

        // TODO:
        // Refactor Resource Manager to take Resource ID as parameter
        // Use Resource Tracker to track resource creation and ensure no duplication
        // Create simple material system -> For each Material -> Pass only texture heap offsets index via Constant Buffer -> Use global ResourceHeapIndex in shader
        // For each entity with Transform and Renderable -> Copy transform to Constant buffers, Bind mesh/materials and Draw

        const auto loadMesh = [&](ResID meshId)
        {
            auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, meshId });
            auto m = asset->DeserializeTo<asset::MeshAsset>();
            return gResourceManager->CreateMesh(m.GetData(), meshId);
        };

        loadMesh(ID("Mesh/cube.obj"));
        mMesh = loadMesh(ID("Mesh/torus.obj"));

        auto material = asset::gMaterialDatabase.mMaterials["Floor"];
        auto bronzeMaterial = asset::gMaterialDatabase.mMaterials["Bronze"];

        auto matHandle = gResourceManager->CreateMaterial(material, ID("Floor"));
        auto matHandle2 = gResourceManager->CreateMaterial(bronzeMaterial, ID("Bronze")); 
        Material* mat = gResourceManager->GetMaterial(matHandle);
        Material* mat2 = gResourceManager->GetMaterial(matHandle2);

        gResourceManager->CreateTexture({ asset::ASSET_TEXTURE, ID("Textures/SunnyCubeMap.dds") });

        mTexture = mat2->mTextures[0];//gResourceManager->CreateTexture(tex.GetDesc(), texId);

        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ ps, shader::PIXEL });
        psoDesc.mVS = gResourceManager->CreateShader({ vs, shader::VERTEX });
        mPso = gResourceManager->CreatePipelineState(psoDesc);
        mReloadManager->RegisterPSO(psoDesc, &mPso);

        mRenderJobHandle = nv::jobs::Execute([this](void* ctx) 
        { 
            nv::log::Info("[Renderer] Start Render Job");
            RenderThreadJob(ctx); 
        });

        auto unloadMaterial = [&](const PBRMaterial& material)
        {
            asset::gpAssetManager->UnloadAsset(material.mAlbedoTexture);
            asset::gpAssetManager->UnloadAsset(material.mNormalTexture);
            asset::gpAssetManager->UnloadAsset(material.mRoughnessTexture);
            asset::gpAssetManager->UnloadAsset(material.mMetalnessTexture);
        };

        unloadMaterial(material);
        unloadMaterial(bronzeMaterial);
    }

    void RenderSystem::Update(float deltaTime, float totalTime)
    {
        mRenderData.QueueRenderData();
    }

    void RenderSystem::Destroy()
    {
        nv::jobs::Wait(mRenderJobHandle);
        gRenderer->Wait();
        Free(mReloadManager);
        Free(mpConstantBufferPool);
    }

    void RenderSystem::OnReload()
    {
    }

    void RenderSystem::QueueReload(Handle<PipelineState>* pso)
    {
        mPsoReloadQueue.Push(pso);
    }

    void RenderSystem::Reload(Handle<PipelineState>* pso)
    {
        // Since the shaders referred by the pso description have the same asset ID, 
        // recreating with the same description would ensure the updated asset data is 
        // used by the shader. This is possible because the shader bytecode directly 
        // points to the asset data. 
        *pso = gResourceManager->RecreatePipelineState(*pso); 
    }

    void RenderSystem::RenderThreadJob(void* ctx)
    {
        while (Instance::GetInstanceState() == INSTANCE_STATE_RUNNING)
        {
            NV_FRAME("RenderThread");
            if (!mPsoReloadQueue.IsEmpty())
            {
                gRenderer->WaitForAllFrames();
                while (!mPsoReloadQueue.IsEmpty())
                {
                    auto pso = mPsoReloadQueue.Pop();
                    Reload(pso);
                }
            }

            UploadDrawData();
            gRenderer->Wait();
            gRenderer->StartFrame();

            Context* ctx = gRenderer->GetContext();
            auto objectCbs = mRenderData.GetObjectDescriptors();
            auto materialCbs = mRenderData.GetMaterialDescriptors();
            const auto bindAndDrawObject = [&](ConstantBufferView objCb, ConstantBufferView matCb, Mesh* mesh)
            {
                ctx->BindConstantBuffer(0, (uint32_t)objCb.mHeapIndex);
                ctx->BindConstantBuffer(1, (uint32_t)matCb.mHeapIndex);
                ctx->SetMesh(mesh);
                for (auto entry : mesh->GetDesc().mMeshEntries)
                {
                    ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
                }
            };

            GPUBeginEvent(ctx, "Frame");

            const auto topology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            const auto renderTarget = gRenderer->GetDefaultRenderTarget();
            const auto depthTarget = gRenderer->GetDefaultDepthTarget();
            const auto gpuHeap = gRenderer->GetGPUDescriptorHeap();
            Handle<Texture> targets[] = { renderTarget };
            Handle<DescriptorHeap> heaps[] = { gpuHeap };

            ctx->SetScissorRect(1, &mRect);
            ctx->SetViewports(1, &mViewport);
            ctx->SetPrimitiveTopology(topology);
            ctx->SetDescriptorHeap({ heaps, _countof(heaps) });

            ctx->SetRenderTarget({ targets, _countof(targets) }, depthTarget);
            ctx->SetPipeline(mPso);
            ctx->Bind(4, BIND_BUFFER, (uint32_t)mFrameCB.mHeapIndex);

            for (size_t i = 0; i < objectCbs.Size(); ++i)
            {
                const auto& objectCb = objectCbs[i];
                const auto& matCb = materialCbs[i];
                const auto mesh = mCurrentRenderData[i].mpMesh;
                if(mesh)
                    bindAndDrawObject(objectCb, matCb, mesh);
            }

            // TODO:
            // Indirect Draw
            GPUEndEvent(ctx);

            gRenderer->EndFrame();
            gRenderer->Present();
            gRenderer->ExecuteQueuedDestroy();
        }
    }

    void RenderSystem::UploadDrawData()
    {
        UpdateRenderData();

        mCamera.SetPosition({ 0,0, -5 });
        mCamera.UpdateViewProjection();
        auto view = mCamera.GetViewTransposed();
        auto proj = mCamera.GetProjTransposed();
        FrameData data = { .View = view, .Projection = proj};
        gRenderer->UploadToConstantBuffer(mFrameCB, (uint8_t*)&data, sizeof(data));

        auto objectCbs = mRenderData.GetObjectDescriptors();
        auto materialCbs = mRenderData.GetMaterialDescriptors();

        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            auto& objectCb = objectCbs[i];
            auto& objdata = mCurrentRenderData[i].mObjectData;
            auto& mat = mCurrentRenderData[i].mpMaterial;
            auto& matCb = materialCbs[i];

            gRenderer->UploadToConstantBuffer(objectCb, (uint8_t*)&objdata, sizeof(ObjectData));

            if (mat)
            {
                MaterialData matData;
                matData.AlbedoOffset = gResourceManager->GetTexture(mat->mTextures[Material::ALBEDO])->GetHeapIndex();
                matData.NormalOffset = gResourceManager->GetTexture(mat->mTextures[Material::NORMAL])->GetHeapIndex();
                matData.RoughnessOffset = gResourceManager->GetTexture(mat->mTextures[Material::ROUGHNESS])->GetHeapIndex();
                matData.MetalnessOffset = gResourceManager->GetTexture(mat->mTextures[Material::METALNESS])->GetHeapIndex();
                gRenderer->UploadToConstantBuffer(matCb, (uint8_t*)&matData, sizeof(MaterialData));
            }
        }
    }

    void RenderSystem::UpdateRenderData()
    {
        if(mRenderData.PopRenderData(mCurrentRenderData))
            mRenderData.GenerateDescriptors(mCurrentRenderData);
    }
}