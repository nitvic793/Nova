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

#include <thread>
#include <functional>

namespace nv::graphics
{
    RenderSystem::RenderSystem(uint32_t width, uint32_t height) :
        mCamera(CameraDesc{ .mWidth = (float)width, .mHeight = (float)height })
    {
    }

    void RenderSystem::Init()
    {
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
            // Copy descriptors (Constant Buffers/Textures/etc) to GPU Heap 
            // Set Viewport/Scissor Rect. 
            // Set Render Target
            // Set Descriptor Heap
            // Set Prim Topology
            // Bind resources - Constant Buffer (Textures later) 
            // Draw call

            Context* ctx = gRenderer->GetContext();
            ctx->SetMesh(mMesh);

            gRenderer->EndFrame();
            gRenderer->Present();
        }

        
    }

    void RenderSystem::UploadDrawData()
    {
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