#include "pch.h"
#include "DebugDrawPass.h"

#include <Engine/EntityComponent.h>
#include <Engine/Camera.h>
#include <Math/Collision.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DebugUI/DebugDraw.h>
#include <DX12/Interop.h>
#include <DX12/ContextDX12.h>
#include <DebugUI/DebugUIPass.h>

#include <EffectPipelineStateDescription.h>
#include <CommonStates.h>
#include <Effects.h>
#include <Types/Serializers.h>
#include <GraphicsMemory.h>
#include <memory>

namespace nv::graphics
{
	using namespace DirectX;
	using namespace debug::dx12;

	std::unique_ptr<PrimitiveBatch<VertexPositionColor>> mBatch;
	std::unique_ptr<BasicEffect> mEffect;
	std::unique_ptr<GraphicsMemory> mGraphicsMemory;


void DebugDrawPass::Init()
{
	auto renderer = (RendererDX12*)gRenderer;
	ID3D12Device* pDevice = ((DeviceDX12*)gRenderer->GetDevice())->GetDevice();

	mGraphicsMemory = std::make_unique<GraphicsMemory>(pDevice);

	auto rtFormat = renderer->GetDefaultRenderTargetFormat();
	auto depthFormat = renderer->GetDepthSurfaceFormat();
	mBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(pDevice);
	RenderTargetState rtState(GetFormat(rtFormat), GetFormat(depthFormat));
	EffectPipelineStateDescription pd(
		&VertexPositionColor::InputLayout,
		CommonStates::Opaque,
		CommonStates::DepthDefault,
		CommonStates::CullNone,
		rtState);
	pd.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	mEffect = std::unique_ptr<BasicEffect>(new BasicEffect(pDevice, EffectFlags::VertexColor, pd));
}

void DebugDrawPass::Execute(const RenderPassData& renderPassData)
{
	if (!IsDebugDrawEnabled())
		return;

	auto renderer = (RendererDX12*)gRenderer;
	auto ctx = (ContextDX12*)renderer->GetContext();
	auto pool = ecs::gComponentManager.GetPool<math::BoundingBox>();
	ecs::EntityComponents<math::BoundingBox> comps;
	pool->GetEntityComponents(comps);

	auto camHandle = GetActiveCamera();
	if (camHandle.IsNull())
		return;

	auto cam = ecs::gEntityManager.GetEntity(camHandle);
	auto camComponent = cam->Get<CameraComponent>();

	auto& camera = camComponent->mCamera;
	auto view = camera.GetView();
	auto proj = camera.GetProjection();
	ID3D12GraphicsCommandList* commandList = ctx->GetCommandList();

	mEffect->SetView(XMLoadFloat4x4(&view));
	mEffect->SetProjection(XMLoadFloat4x4(&proj));
	mEffect->Apply(commandList);

	mBatch->Begin(commandList);

	for (uint32_t i = 0; i < comps.Size(); ++i)
	{
		Draw(mBatch.get(), comps[i].mpComponent->mBounding, Colors::Blue); // BoundingBox
	}

	mBatch->End();

	SetContextDefault(ctx);
}

void DebugDrawPass::Destroy()
{
	mEffect.reset();
	mBatch.reset();
	mGraphicsMemory.reset();
}

GraphicsMemory* GetGraphicsMemory()
{
	return mGraphicsMemory.get();
}

}
