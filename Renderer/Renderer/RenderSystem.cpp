#include "pch.h"
#include "RenderSystem.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Engine/JobSystem.h>
#include <Engine/Instance.h>
#include <Engine/Log.h>
#include <Engine/Transform.h>
#include <Engine/EventSystem.h>
#include <Debug/Profiler.h>

#include <AssetReload.h>
#include <AssetManager.h>
#include <Types/MeshAsset.h>
#include <Types/ShaderAsset.h>
#include <Types/TextureAsset.h>

#include <Renderer/ResourceManager.h>
#include <Renderer/Context.h>
#include <Renderer/PipelineState.h>
#include <Renderer/Mesh.h>
#include <Renderer/Window.h>
#include <Renderer/GPUProfile.h>
#include <Renderer/ConstantBufferPool.h>
#include <Components/Material.h>

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

        auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, ID("Mesh/cone.obj") });
        auto m = asset->DeserializeTo<asset::MeshAsset>();
        mMesh = gResourceManager->CreateMesh(m.GetData());

        PBRMaterial material = 
        { 
            .mAlbedoTexture     = { asset::ASSET_TEXTURE, ID("Textures/floor_albedo.png")},
            .mNormalTexture     = { asset::ASSET_TEXTURE, ID("Textures/floor_normals.png")},
            .mRoughnessTexture  = { asset::ASSET_TEXTURE, ID("Textures/floor_roughness.png")},
            .mMetalnessTexture  = { asset::ASSET_TEXTURE, ID("Textures/floor_metal.png")},
        };

        PBRMaterial bronzeMaterial = 
        { 
            .mAlbedoTexture     = { asset::ASSET_TEXTURE, ID("Textures/bronze_albedo.png")},
            .mNormalTexture     = { asset::ASSET_TEXTURE, ID("Textures/bronze_normals.png")},
            .mRoughnessTexture  = { asset::ASSET_TEXTURE, ID("Textures/bronze_roughness.png")},
            .mMetalnessTexture  = { asset::ASSET_TEXTURE, ID("Textures/bronze_metal.png")},
        };

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
        //std::this_thread::sleep_for(std::chrono::milliseconds(3));
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
            ctx->Bind(0, BIND_BUFFER, (uint32_t)mObjectDrawData.mObjectCBView.mHeapIndex);
            ctx->Bind(4, BIND_BUFFER, (uint32_t)mFrameCB.mHeapIndex);
            ctx->BindConstantBuffer(1, (uint32_t)mObjectDrawData.mMaterialCBView.mHeapIndex);
            ctx->BindTexture(2, mTexture);
            ctx->SetMesh(mMesh);
            auto mesh = gResourceManager->GetMesh(mMesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
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
        mCamera.SetPosition({ 0,0, -5 });
        mCamera.UpdateViewProjection();
        auto view = mCamera.GetViewTransposed();
        auto proj = mCamera.GetProjTransposed();
        FrameData data = { .View = view, .Projection = proj};
        gRenderer->UploadToConstantBuffer(mFrameCB, (uint8_t*)&data, sizeof(data));

        Transform transform = {};
        mObjectDrawData.mData.World = transform.GetTransformMatrixTransposed();
        gRenderer->UploadToConstantBuffer(mObjectDrawData.mObjectCBView, (uint8_t*)&mObjectDrawData.mData, sizeof(ObjectData));

        mObjectDrawData.mMaterial.AlbedoOffset = gResourceManager->GetTexture(mTexture)->GetHeapIndex();
        gRenderer->UploadToConstantBuffer(mObjectDrawData.mMaterialCBView, (uint8_t*)&mObjectDrawData.mMaterial, sizeof(MaterialData));
    }
}