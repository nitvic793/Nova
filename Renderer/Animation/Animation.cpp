#include "pch.h"

#include <Animation/Animation.h>
#include <Debug/Profiler.h>
#include <mutex>

namespace nv::graphics::animation
{
	using namespace DirectX;

	AnimationManager gAnimManager;

	constexpr bool ENABLE_ANIM_ASSERT = false;

#define ANIM_ASSERT(expression) if constexpr(ENABLE_ANIM_ASSERT) assert(expression);

	uint32_t FindPosition(float AnimationTime, const AnimationChannel* channel)
	{
		const auto keySize = channel->PositionKeys.size();
		for (uint32_t i = 0; i < keySize - 1; i++) {
			if (AnimationTime < (float)channel->PositionKeys.at(i + 1).Time) {
				return i;
			}
		}

		ANIM_ASSERT(0);
		return 0;
	}

	uint32_t FindScaling(float AnimationTime, const AnimationChannel* channel)
	{
		const auto size = channel->ScalingKeys.size();
		for (uint32_t i = 0; i < size - 1; i++) {
			if (AnimationTime < (float)channel->ScalingKeys.at(i + 1).Time) {
				return i;
			}
		}

		ANIM_ASSERT(0);
		return 0;
	}

	uint32_t FindRotation(float AnimationTime, const AnimationChannel* channel)
	{
		const auto size = channel->RotationKeys.size();
		for (uint32_t i = 0; i < size - 1; i++) {
			if (AnimationTime < (float)channel->RotationKeys.at(i + 1).Time) {
				return i;
			}
		}

		ANIM_ASSERT(0);
		return 0;
	}

	XMVECTOR InterpolatePosition(float animTime, const AnimationChannel* channel)
	{
		auto outFloat3 = XMFLOAT3();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->PositionKeys.size() == 1) {
			Out = XMLoadFloat3(&channel->PositionKeys[0].Value);
			return Out;
		}

		uint32_t PositionIndex = FindPosition(animTime, channel);
		uint32_t NextPositionIndex = (PositionIndex + 1);
		ANIM_ASSERT(NextPositionIndex < channel->PositionKeys.size());

		float DeltaTime = (float)(channel->PositionKeys[NextPositionIndex].Time - channel->PositionKeys[PositionIndex].Time);
		float Factor = (animTime - (float)channel->PositionKeys[PositionIndex].Time) / DeltaTime;
		ANIM_ASSERT(Factor >= 0.0f && Factor <= 1.0f);

		auto Start = XMLoadFloat3(&channel->PositionKeys[PositionIndex].Value);
		auto End = XMLoadFloat3(&channel->PositionKeys[NextPositionIndex].Value);
		auto Delta = End - Start;
		Out = Start + Factor * Delta;
		return Out;
	}


	XMVECTOR InterpolateScaling(float animTime, const AnimationChannel* channel)
	{
		auto outFloat3 = XMFLOAT3();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->ScalingKeys.size() == 1) {
			Out = XMLoadFloat3(&channel->ScalingKeys[0].Value);
			return Out;
		}

		uint32_t ScaleIndex = FindScaling(animTime, channel);
		uint32_t NextScaleIndex = (ScaleIndex + 1);
		ANIM_ASSERT(NextScaleIndex < channel->ScalingKeys.size());

		float DeltaTime = (float)(channel->ScalingKeys[NextScaleIndex].Time - channel->ScalingKeys[ScaleIndex].Time);
		float Factor = (animTime - (float)channel->ScalingKeys[ScaleIndex].Time) / DeltaTime;
		ANIM_ASSERT(Factor >= 0.0f && Factor <= 1.0f);

		auto Start = XMLoadFloat3(&channel->ScalingKeys[ScaleIndex].Value);
		auto End = XMLoadFloat3(&channel->ScalingKeys[NextScaleIndex].Value);
		auto Delta = End - Start;
		Out = Start + Factor * Delta;
		//XMStoreFloat3(&outFloat3, Out);
		return Out;
	}

	XMFLOAT4 InterpolateRotation(float animTime, const AnimationChannel* channel)
	{
		auto outFloat4 = XMFLOAT4();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->RotationKeys.size() == 1) {
			//Out = XMLoadFloat4(&channel->RotationKeys[0].Value);
			return channel->RotationKeys[0].Value;
		}

		uint32_t RotationIndex = FindRotation(animTime, channel);
		uint32_t NextRotationIndex = (RotationIndex + 1);
		ANIM_ASSERT(NextRotationIndex < channel->RotationKeys.size());

		float DeltaTime = (float)(channel->RotationKeys[NextRotationIndex].Time - channel->RotationKeys[RotationIndex].Time);
		float Factor = (animTime - (float)channel->RotationKeys[RotationIndex].Time) / DeltaTime;
		ANIM_ASSERT(Factor >= 0.0f && Factor <= 1.0f);

		auto StartRotationQ = XMLoadFloat4(&channel->RotationKeys[RotationIndex].Value);
		auto EndRotationQ = XMLoadFloat4(&channel->RotationKeys[NextRotationIndex].Value);
		Out = XMQuaternionSlerp((StartRotationQ), (EndRotationQ), Factor);
		Out = XMQuaternionNormalize(Out);
		XMStoreFloat4(&outFloat4, Out);
		return outFloat4;
	}

	void AnimationManager::Register(Handle<Mesh> meshHandle, const MeshAnimNodeData& data)
	{
		mMeshAnimNodeMap[meshHandle.mHandle] = ScopedPtr<MeshAnimNodeData, true>( new MeshAnimNodeData(data));
	}

	void AnimationManager::Register(uint64_t handle, Mesh* pMesh)
	{
		const auto& boneData = pMesh->GetBoneData();
		const auto size = boneData.mBoneInfoList.size();

		mAnimInstanceMap[handle] = MakeScoped<AnimationInstanceData, true>();
		auto& instance = *mAnimInstanceMap[handle];

		memset((void*)&instance.mArmatureConstantBuffer, 0, sizeof(PerArmature));
		memcpy(&instance.mBoneInfoList[0], boneData.mBoneInfoList.data(), size * sizeof(BoneInfo));
		instance.mBoneInfoSize = (uint32_t)size;
	}

	void AnimationManager::Register(const AnimationStore& store)
	{
		mAnimStore.Animations.insert(mAnimStore.Animations.end(), store.Animations.begin(), store.Animations.end());
		for (uint32_t i = 0; i < mAnimStore.Animations.size(); ++i)
		{
			const auto& anim = mAnimStore.Animations[i];
			mAnimStore.AnimationIndexMap[anim.AnimationName] = i;
		}
	}

	void AnimationManager::Unregister(uint64_t handle)
	{
        mAnimInstanceMap.erase(handle);
	}

	AnimationInstanceData& AnimationManager::GetInstance(uint64_t handle)
	{
		return *mAnimInstanceMap.at(handle);
	}

	MeshAnimNodeData& AnimationManager::GetMeshAnimNodeData(Handle<Mesh> meshHandle)
	{
		return *mMeshAnimNodeMap.at(meshHandle.mHandle);
	}


	Animation* AnimationManager::GetAnimation(const std::string& animName)
	{
		if (mAnimStore.AnimationIndexMap.find(animName) == mAnimStore.AnimationIndexMap.end())
		{
			return nullptr;
		}

		return &mAnimStore.Animations[mAnimStore.AnimationIndexMap[animName]];
	}

	const Animation& AnimationManager::GetAnimation(uint32_t index) const
	{
		return mAnimStore.Animations[index];
	}

	const std::string& AnimationManager::GetAnimationName(uint32_t index) const
	{
		return mAnimStore.Animations[index].AnimationName;
	}

	int32_t AnimationManager::GetChannelIndex(uint32_t animationIndex, const std::string& node) const
	{
		auto& map = mAnimStore.Animations[animationIndex].NodeChannelMap;
		auto index = -1;
		while (map.find(node) == map.end())
		{
			return index;
		}

		index = map.find(node)->second;
		return index;
	}

	const AnimationChannel* AnimationManager::GetChannel(uint32_t animIndex, const std::string& node) const
	{
		auto channelIndex = GetChannelIndex(animIndex, node);
		if (channelIndex == -1)
		{
			return nullptr;
		}

		return &mAnimStore.Animations[animIndex].Channels[channelIndex];
	}

	std::vector<std::string_view> AnimationManager::GetAnimationNames() const
	{
		std::vector<std::string_view> animNames;
		for (const auto& anim : mAnimStore.AnimationIndexMap)
		{
			animNames.push_back(anim.first.c_str());
		}

		return animNames;
	}

	uint32_t AnimationManager::GetAnimationCount() const
	{
		return (uint32_t)mAnimStore.Animations.size();
	}

	uint32_t animation::AnimationManager::GetAnimationIndex(const char* pName) const
	{
		return mAnimStore.AnimationIndexMap.at(pName);
	}

	bool AnimationManager::IsAnimationIndexValid(uint32_t animationIndex) const
	{
		return (animationIndex <= mAnimStore.Animations.size() - 1 && animationIndex >= 0);
	}

	NV_MUTEX(std::mutex, gMutex);

	void AnimationManager::Lock()
	{
		gMutex.lock();
	}

	void animation::AnimationManager::Unlock()
	{
		gMutex.unlock();
	}
}