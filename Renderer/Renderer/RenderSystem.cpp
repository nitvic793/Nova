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

#include <RenderPasses/RTCompute.h>
#include <RenderPasses/ForwardPass.h>
#include <RenderPasses/RaytracePass.h>
#include <RenderPasses/Skybox.h>

#include <DebugUI/DebugUIPass.h>

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

    void LoadResources()
    {
        const auto loadMesh = [&](ResID meshId)
        {
            auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, meshId });
            auto m = asset->DeserializeTo<asset::MeshAsset>();
            return gResourceManager->CreateMesh(m.GetData(), meshId);
        };

        nv::Vector<PBRMaterial> materials;
        const auto loadMaterials = [&]()
        {
            for (const auto& mat : asset::gMaterialDatabase.mMaterials)
            {
                gResourceManager->CreateMaterial(mat.second, ID(mat.first.c_str()));
                materials.Push(mat.second);
            }
        };

        auto unloadMaterialAssets = [&](const PBRMaterial& material)
        {
            asset::gpAssetManager->UnloadAsset(material.mAlbedoTexture);
            asset::gpAssetManager->UnloadAsset(material.mNormalTexture);
            asset::gpAssetManager->UnloadAsset(material.mRoughnessTexture);
            asset::gpAssetManager->UnloadAsset(material.mMetalnessTexture);
        };

        loadMesh(ID("Mesh/cube.obj"));
        loadMesh(ID("Mesh/torus.obj"));
        loadMaterials();
        gResourceManager->CreateTexture({ asset::ASSET_TEXTURE, ID("Textures/SunnyCubeMap.dds") });

        for (const auto& mat : materials)
            unloadMaterialAssets(mat);
    }

    void RenderSystem::Init()
    {
        mpConstantBufferPool = Alloc<ConstantBufferPool>();
        gpConstantBufferPool = mpConstantBufferPool;

        mFrameCB = mpConstantBufferPool->GetConstantBuffer<FrameData>();
        mHeapStateCB = mpConstantBufferPool->GetConstantBuffer<HeapState, 4>();
        mObjectDrawData.mObjectCBView = mpConstantBufferPool->GetConstantBuffer<ObjectData>();
        mObjectDrawData.mMaterialCBView = mpConstantBufferPool->GetConstantBuffer<MaterialData>();

        LoadResources();
        gRenderer->GetDevice()->InitRaytracingContext();

        mRenderPasses.Emplace(Alloc<RTCompute>());
        mRenderPasses.Emplace(Alloc<ForwardPass>());
        mRenderPasses.Emplace(Alloc<Skybox>());
        //mRenderPasses.Emplace(Alloc<RaytracePass>());
        mRenderPasses.Emplace(Alloc<DebugUIPass>());

        for (auto& pass : mRenderPasses)
        {
            pass->Init();
        }

        //mReloadManager->RegisterPSO(psoDesc, &mPso);

        mRenderJobHandle = nv::jobs::Execute([this](void* ctx) 
        { 
            log::Info("[Renderer] Start Render Job");
            RenderThreadJob(ctx); 
        });
    }

    void RenderSystem::Update(float deltaTime, float totalTime)
    {
        mRenderData.QueueRenderData();
    }

    void RenderSystem::Destroy()
    {
        nv::jobs::Wait(mRenderJobHandle);
        gRenderer->Wait();
        for (auto& pass : mRenderPasses)
        {
            pass->Destroy();
        }

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

            UpdateRenderData(); // Generates descriptors
            gRenderer->Wait();
            gRenderer->StartFrame();
            UploadDrawData();

            {
                Context* ctx = gRenderer->GetContext();
                GPUBeginEvent(ctx, "Frame");
                NV_EVENT("Renderer/Frame");

                const RenderPassData data =
                {
                    .mFrameDataCBV      = mFrameCB,
                    .mRenderData        = mCurrentRenderData,
                    .mRenderDataArray   = mRenderData,
                    .mHeapStateCB       = mHeapStateCB
                };

                for (auto& pass : mRenderPasses)
                {
                    GPUBeginEvent(ctx, pass->GetName());
                    pass->Execute(data);
                    GPUEndEvent(ctx);
                }

                // TODO:
                // Indirect Draw
                GPUEndEvent(ctx);
            }

            gRenderer->EndFrame();
            gRenderer->Present();
            gRenderer->ExecuteQueuedDestroy();
        }
    }

    void RenderSystem::UploadDrawData()
    {
        auto cams = ecs::gComponentManager.GetComponents<CameraComponent>();
        if (cams.Size() == 0)
            return;

        auto& camera = cams[0].mCamera;

        mCamera.SetPosition({ 0,0, -5 });
        mCamera.UpdateViewProjection();
        auto view = camera.GetViewTransposed();
        auto proj = camera.GetProjTransposed();
        auto projI = camera.GetProjInverseTransposed();
        auto viewI = camera.GetViewInverseTransposed();
        auto dirLights = ecs::gComponentManager.GetComponents<DirectionalLight>();
        
        FrameData data = { .View = view, .Projection = proj, .ViewInverse = viewI, .ProjectionInverse = projI };
        if (dirLights.Size() > 0)
        {
            for (uint32_t i = 0; i < dirLights.Size() && i < MAX_DIRECTIONAL_LIGHTS; ++i)
            {
                data.DirLights[i] = dirLights[i];
            }

            data.DirLightsCount = std::min((uint32_t)MAX_DIRECTIONAL_LIGHTS, (uint32_t)dirLights.Size());
        }

        gRenderer->UploadToConstantBuffer(mFrameCB, (uint8_t*)&data, sizeof(data));

        auto& heapState = gRenderer->GetGPUHeapState();
        HeapState state = { .ConstBufferOffset = heapState.mConstBufferOffset, .TextureOffset = heapState.mTextureOffset };
        gRenderer->UploadToConstantBuffer(mHeapStateCB, (uint8_t*)&state, sizeof(state));

        auto objectCbs = mRenderData.GetObjectDescriptors();
        auto materialCbs = mRenderData.GetMaterialDescriptors();

        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            auto& objectCb = objectCbs[i];
            auto& objdata = mCurrentRenderData[i].mObjectData;
            auto& mat = mCurrentRenderData[i].mpMaterial;
            auto& matCb = materialCbs[i];

            objdata.MaterialIndex = gRenderer->GetHeapIndex(matCb);
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