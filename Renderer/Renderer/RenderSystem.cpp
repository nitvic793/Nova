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
#include <Renderer/GlobalRenderSettings.h>
#include <Renderer/GPUProfile.h>
#include <Renderer/ConstantBufferPool.h>
#include <Components/Material.h>
#include <Components/Renderable.h>

#include <RenderPasses/RTCompute.h>
#include <RenderPasses/ForwardPass.h>
#include <RenderPasses/RaytracePass.h>
#include <RenderPasses/Skybox.h>
#include <RenderPasses/DebugDrawPass.h>
#include <RenderPasses/GBuffer.h>

#include <DebugUI/DebugUIPass.h>

#include <thread>
#include <functional>

namespace nv::graphics
{
    class RenderReloadManager : public IRenderReloadManager
    {
    public:
        RenderReloadManager(RenderSystem* renderSystem) :
            mRenderSystem(renderSystem)
        {}

        void RegisterPSO(const PipelineStateDesc& desc, Handle<PipelineState> pso) override
        {
            auto psId = !desc.mPS.IsNull() ? gResourceManager->GetShader(desc.mPS)->GetDesc().mShader : asset::AssetID{ .mId = RES_ID_NULL };
            auto vsId = !desc.mVS.IsNull() ? gResourceManager->GetShader(desc.mVS)->GetDesc().mShader : asset::AssetID{ .mId = RES_ID_NULL };
            auto csId = !desc.mCS.IsNull() ? gResourceManager->GetShader(desc.mCS)->GetDesc().mShader : asset::AssetID{ .mId = RES_ID_NULL };

            if (psId.mId != RES_ID_NULL)
                mPsoShaderMap[psId.mId].push_back(pso);
            if (vsId.mId != RES_ID_NULL)
                mPsoShaderMap[vsId.mId].push_back(pso);
            if (csId.mId != RES_ID_NULL)
                mPsoShaderMap[csId.mId].push_back(pso);
        }

        void OnReload(asset::AssetReloadEvent* event)
        {
            if (event->mAssetId.mType == asset::ASSET_SHADER)
            {
                auto it = mPsoShaderMap.find(event->mAssetId.mId);
                if(it == mPsoShaderMap.end())
                    return;
                const auto& PSOs = it->second;
                for(const auto& pso : PSOs)
                    mRenderSystem->QueueReload(pso);
            }
        }

    private:
        HashMap<uint64_t, std::vector<Handle<PipelineState>>> mPsoShaderMap;
        RenderSystem* mRenderSystem;
    };

    RenderSystem::RenderSystem(uint32_t width, uint32_t height) :
        mCamera(CameraDesc{ .mWidth = (float)width, .mHeight = (float)height }),
        mRect(),
        mViewport(),
        mpConstantBufferPool(nullptr)
    {
        mReloadManager = Alloc<RenderReloadManager>(SystemAllocator::gPtr, this);
        SetRenderReloadManager(mReloadManager);
        gEventBus.Subscribe(mReloadManager, &RenderReloadManager::OnReload);
    }

    void LoadResources()
    {
        const auto loadMesh = [&](ResID meshId)
        {
            auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, meshId });
            auto m = asset->DeserializeTo<asset::MeshAsset>();
            auto handle = gResourceManager->CreateMesh(m.GetData(), meshId);
            m.Register(handle);
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

        //loadMesh(ID("Mesh/cube.obj"));
        loadMesh(ID("Mesh/torus.obj"));
        loadMesh(ID("Mesh/cone.obj"));
        loadMesh(ID("Mesh/plane.obj"));
        loadMesh(ID("Mesh/male.fbx"));
        loadMesh(ID("Mesh/anim_running.fbx"));
        loadMesh(ID("Mesh/anim_idle.fbx"));
        loadMesh(ID("Mesh/knight.fbx"));
        loadMaterials();

        gResourceManager->CreateTexture({ asset::ASSET_TEXTURE, ID("Textures/SunnyCubeMap.dds") });
        gResourceManager->CreateTexture({ asset::ASSET_TEXTURE, ID("Textures/Sky.hdr") });
        gResourceManager->CreateTexture({ asset::ASSET_TEXTURE, ID("Textures/bluenoise256.png") });

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

        mRenderPasses.Emplace(Alloc<GBuffer>());
        mRenderPasses.Emplace(Alloc<ForwardPass>());
        mRenderPasses.Emplace(Alloc<RTCompute>());

        mRenderPasses.Emplace(Alloc<Skybox>());
        mRenderPasses.Emplace(Alloc<DebugDrawPass>());
        //mRenderPasses.Emplace(Alloc<RaytracePass>());
        mRenderPasses.Emplace(Alloc<DebugUIPass>());

        for (auto& pass : mRenderPasses)
        {
            pass->Init();
        }

        //mReloadManager->RegisterPSO(psoDesc, &mPso);

        mRenderJobHandle = nv::jobs::Execute([this](void* ctx) 
        { 
            NV_THREAD("RenderThread");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
        gContext.mpInstance->Notify();
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

    void RenderSystem::QueueReload(Handle<PipelineState> pso)
    {
        mPsoReloadQueue.Push(pso);
    }

    void RenderSystem::Reload(Handle<PipelineState> pso)
    {
        // Since the shaders referred by the pso description have the same asset ID, 
        // recreating with the same description would ensure the updated asset data is 
        // used by the shader. This is possible because the shader bytecode directly 
        // points to the asset data. 
        gResourceManager->RecreatePipelineState(pso); 
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
            if (mRenderData.GetRenderDataQueueSize() > 1 && gContext.mpInstance->GetInstanceState() == INSTANCE_STATE_RUNNING)
            {
                gContext.mpInstance->Wait(); // Wait until main thread notifies us it's done
            }
        }
    }

    void RenderSystem::UploadDrawData()
    {
        NV_EVENT("Renderer/UploadDrawData");

        auto camHandle = GetActiveCamera();
        if (camHandle.IsNull())
            return;

        auto cam = ecs::gEntityManager.GetEntity(camHandle);
        auto camComponent = cam->Get<CameraComponent>();

        auto& camera = camComponent->mCamera;

       // mCamera.SetPosition({ 0,0, -5 });
       // mCamera.UpdateViewProjection();
        auto view = camera.GetViewTransposed();
        auto proj = camera.GetProjTransposed();
        auto projI = camera.GetProjInverseTransposed();
        auto viewI = camera.GetViewInverseTransposed();
        auto dirLights = ecs::gComponentManager.GetComponents<DirectionalLight>();
        
        FrameData data = 
        { 
            .View                   = view, 
            .Projection             = proj, 
            .PrevView               = camera.GetPreviousViewTransposed(),
            .PrevProjection         = camera.GetPreviousProjectionTransposed(),
            .ViewInverse            = viewI, 
            .ProjectionInverse      = projI, 
            .ViewProjectionInverse  = camera.GetViewProjInverseTransposed(),
            .CameraPosition         = camera.GetPosition(),
            .NearZ                  = camera.GetNearZ(),
            .FarZ                   = camera.GetFarZ(),
            .GBufferAIdx            = gResourceManager->GetTexture(ID("GBuffer/GBufferMatA_SRV"))->GetHeapIndex(),
            .GBufferBIdx            = gResourceManager->GetTexture(ID("GBuffer/GBufferMatB_SRV"))->GetHeapIndex(),
            .GBufferCIdx            = gResourceManager->GetTexture(ID("GBuffer/GBufferWorldPos_SRV"))->GetHeapIndex(),
            .GBufferDIdx            = gResourceManager->GetTexture(ID("GBuffer/GBufferVelocity_SRV"))->GetHeapIndex(),
            .GBufferDepthIdx		= gResourceManager->GetTexture(ID("GBuffer/GBufferDepth_SRV"))->GetHeapIndex()
        };

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

        uint32_t boneIdx = 0;
        auto objectCbs = mRenderData.GetObjectDescriptors();
        auto materialCbs = mRenderData.GetMaterialDescriptors();
        auto boneCbs = mRenderData.GetBoneDescriptors();

        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            auto& objectCb = objectCbs[i];
            auto& objdata = mCurrentRenderData[i].mObjectData;
            auto& mat = mCurrentRenderData[i].mpMaterial;
            auto& bones = mCurrentRenderData[i].mpBones;
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

        {
            NV_EVENT("Renderer/UploadBoneTransforms");
            animation::gAnimManager.Lock();
            for (size_t i = 0; i < objectCbs.Size(); ++i)
            {
                auto& bones = mCurrentRenderData[i].mpBones;
                if (bones)
                {
                    auto boneCb = boneCbs[boneIdx];
                    gRenderer->UploadToConstantBuffer(boneCb, (uint8_t*)&bones->Bones[0], sizeof(PerArmature));
                    boneIdx++;
                }
            }
            animation::gAnimManager.Unlock();
        }
    }

    void RenderSystem::UpdateRenderData()
    {
        if(mRenderData.PopRenderData(mCurrentRenderData))
            mRenderData.GenerateDescriptors(mCurrentRenderData);
    }
}