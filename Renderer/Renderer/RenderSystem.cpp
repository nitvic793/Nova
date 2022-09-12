#include "pch.h"
#include "RenderSystem.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Engine/JobSystem.h>
#include <Engine/Instance.h>
#include <Engine/Log.h>
#include <Engine/Transform.h>
#include <Debug/Profiler.h>

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
    RenderSystem::RenderSystem(uint32_t width, uint32_t height) :
        mCamera(CameraDesc{ .mWidth = (float)width, .mHeight = (float)height }),
        mRect(),
        mViewport()
    {
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

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ shader::PIXEL, asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultPS.hlsl") } });
        psoDesc.mVS = gResourceManager->CreateShader({ shader::VERTEX, asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") } });
        mPso = gResourceManager->CreatePipelineState(psoDesc);

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
    }

    void RenderSystem::OnReload()
    {
    }

    void RenderSystem::RenderThreadJob(void* ctx)
    {
        while (Instance::GetInstanceState() == INSTANCE_STATE_RUNNING)
        {
            NV_FRAME("RenderThread");
            UploadDrawData();
            gRenderer->Wait();
            gRenderer->StartFrame();

            // TODO:
            // Bind resources - Constant Buffer (Textures later) 
            // Draw call
            

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
            ctx->Bind(0, BIND_BUFFER, mObjectDrawData.mCBView.mHeapIndex);
            ctx->Bind(4, BIND_BUFFER, mFrameCB.mHeapIndex);
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