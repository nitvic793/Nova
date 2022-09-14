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

#include <Renderer/ResourceManager.h>
#include <Renderer/Context.h>
#include <Renderer/PipelineState.h>
#include <Renderer/Mesh.h>
#include <Renderer/Window.h>

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
        mViewport()
    {
        mReloadManager = Alloc<RenderReloadManager>(SystemAllocator::gPtr, this);
        gEventBus.Subscribe(mReloadManager, &RenderReloadManager::OnReload);
    }

    void RenderSystem::Init()
    {
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

        mFrameCB = gRenderer->CreateConstantBuffer(sizeof(FrameData));
        mObjectDrawData.mCBView = gRenderer->CreateConstantBuffer(sizeof(ObjectData));

        // Test Mesh
        auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, ID("Mesh/cube.obj") });
        asset::MeshAsset m;
        asset->DeserializeTo(m);
        mMesh = gResourceManager->CreateMesh(m.GetData());

        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ shader::PIXEL, ps });
        psoDesc.mVS = gResourceManager->CreateShader({ shader::VERTEX, vs });
        mPso = gResourceManager->CreatePipelineState(psoDesc);
        mReloadManager->RegisterPSO(psoDesc, &mPso);

        mRenderJobHandle = nv::jobs::Execute([this](void* ctx) 
        { 
            nv::log::Info("[Renderer] Start Render Job");
            RenderThreadJob(ctx); 
        });
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

            const auto topology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            const auto renderTarget = gRenderer->GetDefaultRenderTarget();
            const auto depthTarget = gRenderer->GetDefaultDepthTarget();
            const auto gpuHeap = gRenderer->GetGPUDescriptorHeap();
            Handle<Texture> targets[] = { renderTarget };
            Handle<DescriptorHeap> heaps[] = { gpuHeap };

            Context* ctx = gRenderer->GetContext();

            ctx->SetScissorRect(1, &mRect);
            ctx->SetViewports(1, &mViewport);
            ctx->SetPrimitiveTopology(topology);
            ctx->SetDescriptorHeap({ heaps, _countof(heaps) });

            ctx->SetRenderTarget({ targets, _countof(targets) }, depthTarget);
            ctx->SetPipeline(mPso);
            ctx->Bind(0, BIND_BUFFER, (uint32_t)mObjectDrawData.mCBView.mHeapIndex);
            ctx->Bind(4, BIND_BUFFER, (uint32_t)mFrameCB.mHeapIndex);
            ctx->SetMesh(mMesh);
            auto mesh = gResourceManager->GetMesh(mMesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
            }

            gRenderer->EndFrame();
            gRenderer->Present();
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
        gRenderer->UploadToConstantBuffer(mObjectDrawData.mCBView, (uint8_t*)&mObjectDrawData.mData.World, sizeof(ObjectData));
    }
}