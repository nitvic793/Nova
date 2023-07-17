#include "pch.h"

#include <Animation/AnimationSystem.h>
#include <Animation/Animation.h>
#include <Components/Renderable.h>
#include <Renderer/ResourceManager.h>
#include <Engine/JobSystem.h>
#include <Debug/Profiler.h>

namespace nv::graphics::animation
{
	using namespace components;

	void ReadNodeHeirarchy(AnimationComponent& animComponent, const Animation& animation, const MeshAnimNodeData& nodeData, AnimationInstanceData& instanceData, const MeshBoneDesc& boneDesc, float animationTime);
	void BoneTransform(AnimationComponent& animComponent, AnimationInstanceData& instanceData, const Animation& animation, const MeshAnimNodeData& nodeData, const MeshBoneDesc& boneDesc);

	void AnimationSystem::Init()
	{
	}

	void AnimationSystem::Update(float deltaTime, float totalTime)
	{
		NV_EVENT("AnimationSystem/Update")
		auto pComponentPool = ecs::gComponentManager.GetPool<AnimationComponent>();
		if (!pComponentPool)
			return;

		nv::Vector<Handle<jobs::Job>> jobHandles;
		nv::Vector<AnimationInstanceData> animInstances;
		ecs::EntityComponents<AnimationComponent> components;
		pComponentPool->GetEntityComponents(components);

		for (size_t i = 0; i < components.mComponents.size(); ++i)
		{
			AnimationComponent* pComp = components.mComponents[i];
			if (!pComp->mIsPlaying)
				return;

			Handle<ecs::Entity> entityHandle = components.mEntities[i];
			auto pEntity = ecs::gEntityManager.GetEntity(entityHandle);
			auto& instanceData = gAnimManager.GetInstance(entityHandle);
			auto meshHandle = pEntity->Get<components::Renderable>()->mMesh;
			auto pMesh = graphics::gResourceManager->GetMesh(meshHandle);

			pComp->mTotalTime += deltaTime * pComp->mAnimationSpeed;
			auto& instance = gAnimManager.GetInstance(entityHandle);
			auto& animation = gAnimManager.GetAnimation(pComp->mCurrentAnimationIndex);
			auto& nodeData = gAnimManager.GetMeshAnimNodeData(meshHandle);
			auto& boneDesc = pMesh->GetBoneData();

			auto& copyInstance = animInstances.Emplace();
			copyInstance = instance;

			auto handle = jobs::Execute([&](void*)
			{
				BoneTransform(*pComp, copyInstance, animation, nodeData, boneDesc);
			});

			jobHandles.Push(handle);
		}

		for (auto& handle : jobHandles)
		{
			jobs::Wait(handle);
		}

		{
			NV_EVENT("AnimationSystem/CopyAnimInstanceData");
			gAnimManager.Lock();
			for (size_t i = 0; i < components.mEntities.size(); ++i)
			{
				auto& instance = gAnimManager.GetInstance(components.mEntities[i]);
				instance = animInstances[i];
			}

			gAnimManager.Unlock();
		}


	}

	void AnimationSystem::Destroy()
	{
	}

	void BoneTransform(AnimationComponent& animComponent, AnimationInstanceData& instanceData, const Animation& animation, const MeshAnimNodeData& nodeData, const MeshBoneDesc& boneDesc)
	{
		NV_EVENT("AnimationSystem/BoneTransform")
		float totalTime = animComponent.mTotalTime;
		float TicksPerSecond = (float)(animation.TicksPerSecond != 0 ? animation.TicksPerSecond : 25.0f);
		float TimeInTicks = totalTime * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)animation.Duration);
		ReadNodeHeirarchy(animComponent, animation, nodeData, instanceData, boneDesc, AnimationTime);

		{
			NV_EVENT("AnimationSystem/CopyBoneTransforms");
			for (uint32_t i = 0; i < instanceData.mBoneInfoSize; i++)
			{
				float4x4 finalTransform;
				XMStoreFloat4x4(&finalTransform, XMMatrixTranspose(XMLoadFloat4x4(&instanceData.mBoneInfoList[i].FinalTransform)));
				instanceData.mArmatureConstantBuffer.Bones[i] = finalTransform;
			}
		}
	}

	void ReadNodeHeirarchy(AnimationComponent& animComponent, const Animation& animation, const MeshAnimNodeData& nodeData, AnimationInstanceData& instanceData, const MeshBoneDesc& boneDesc, float animationTime)
	{
		NV_EVENT("AnimationSystem/ReadNodeHeirarchy");

		XMFLOAT4X4 identity;
		XMFLOAT4X4 globalFloat4x4;
		std::stack<const std::string*> nodeQueue;
		std::stack<XMFLOAT4X4> transformationQueue;

		uint32_t animationIndex = animComponent.mCurrentAnimationIndex;
		XMMATRIX globalInverse = XMLoadFloat4x4(&nodeData.GlobalInverseTransform);
		XMMATRIX rootTransform = XMMatrixIdentity();

		const std::string& rootNode = nodeData.RootNode;

		XMStoreFloat4x4(&identity, rootTransform);
		nodeQueue.push(&rootNode);
		transformationQueue.push(identity);

		while (!nodeQueue.empty())
		{
			NV_EVENT("AnimationSystem/Interpolation");

			const auto& node = nodeQueue.top();
			auto parentTransformation = XMLoadFloat4x4(&transformationQueue.top());
			auto nodeTransformation = XMLoadFloat4x4(&nodeData.NodeTransformsMap.find(*node)->second);

			nodeQueue.pop();
			transformationQueue.pop();

			const AnimationChannel* anim = gAnimManager.GetChannel(animationIndex, *node);
			if (anim != nullptr)
			{
				auto s = InterpolateScaling(animationTime, anim);
				auto scaling = XMMatrixScalingFromVector(s);

				auto r = InterpolateRotation(animationTime, anim);
				auto rotation = XMMatrixRotationQuaternion(XMVectorSet(r.y, r.z, r.w, r.x));
				//auto rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&r));

				auto t = InterpolatePosition(animationTime, anim);
				auto translation = XMMatrixTranslationFromVector(t);

				nodeTransformation = XMMatrixAffineTransformation(s, XMVectorSet(r.y, r.z, r.w, r.x), XMVectorSet(r.y, r.z, r.w, r.x), t);
				//nodeTransformation = XMMatrixAffineTransformation(s, XMVectorSet(r.x, r.y, r.z, r.w), XMLoadFloat4(&r), t);
				//nodeTransformation += scaling * rotation * translation;
			}

			auto globalTransformation = nodeTransformation * parentTransformation;
			if (boneDesc.mBoneMapping.find(*node) != boneDesc.mBoneMapping.end())
			{
				uint32_t BoneIndex = boneDesc.mBoneMapping.find(*node)->second;
				auto finalTransform = XMMatrixTranspose(XMLoadFloat4x4(&instanceData.mBoneInfoList[BoneIndex].OffsetMatrix)) * globalTransformation * globalInverse;
				XMStoreFloat4x4(&instanceData.mBoneInfoList[BoneIndex].FinalTransform, finalTransform);
			}

			const auto& children = nodeData.NodeHeirarchy.find(*node)->second;
			for (int i = (int)children.size() - 1; i >= 0; --i)
			{
				XMStoreFloat4x4(&globalFloat4x4, globalTransformation);
				nodeQueue.push(&children[i]);
				transformationQueue.push(globalFloat4x4);
			}
		}
	}
}