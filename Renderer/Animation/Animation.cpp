#include "pch.h"

#include <Animation/Animation.h>
#include <Debug/Profiler.h>
#include <mutex>
#include <immintrin.h>

namespace nv::graphics::animation
{
	using namespace DirectX;

	AnimationManager gAnimManager;

	constexpr bool ENABLE_ANIM_ASSERT = false;

#if __AVX2__
#define ENABLE_SIMD 0
#else
#define ENABLE_SIMD 0
#endif

#define ANIM_ASSERT(expression) if constexpr(ENABLE_ANIM_ASSERT) assert(expression);
	template<typename T>
	concept KeyType = requires(T a)
	{
		{ T::Time } -> std::convertible_to<double>;
	};

    template<KeyType T>
	static uint32_t FindIndex(float animTime, std::vector<T> keys, uint32_t& cursor)
	{
#if !ENABLE_SIMD
        const auto size = keys.size();
		if(cursor < size - 1 && animTime >= (float)keys.at(cursor).Time && animTime < (float)keys.at(cursor + 1).Time)
		{
			return cursor;
		}

		if(cursor < size - 1 && animTime >= (float)keys.at(cursor + 1).Time)
		{
			cursor++;
		}
		else
		{
			cursor = 0;
		}

        for (uint32_t i = cursor; i < size - 1; i++) 
		{
            if (animTime < (float)keys.at(i + 1).Time) 
			{
				cursor = i;
                return i;
            }
        }
#else // WIP SIMD
        const auto size = keys.size();
        const auto simdSize = size - 1;
        const auto simdSize8 = (uint32_t)simdSize / 8;

        for (uint32_t i = 0; i < simdSize8; i++) 
		{
            const auto index = i * 8;
            const auto time0 = (float)keys.at(index + 1).Time;
            const auto time1 = (float)keys.at(index + 2).Time;
            const auto time2 = (float)keys.at(index + 3).Time;
            const auto time3 = (float)keys.at(index + 4).Time;
            const auto time4 = (float)keys.at(index + 5).Time;
            const auto time5 = (float)keys.at(index + 6).Time;
            const auto time6 = (float)keys.at(index + 7).Time;
            const auto time7 = (float)keys.at(index + 8).Time;

            const float times[8] = { time0, time1, time2, time3, time4, time5, time6, time7 };

            __m256 time = _mm256_load_ps(&times[0]);
            // less than simd op
            __m256 mask = _mm256_cmp_ps(time, _mm256_broadcast_ss(&animTime), _CMP_LT_OQ);
			
            uint32_t maskInt = _mm256_movemask_ps(mask);
            if (maskInt != 0) 
			{
				uint32_t result = index + 8 - (32u - _lzcnt_u32(maskInt));
                assert(result < size);
                return result;
            }
        }

        for (uint32_t i = simdSize8 * 8; i < size - 1; i++) 
		{
            if (animTime < (float)keys.at(i + 1).Time) 
			{
                return i;
            }
        }
#endif

		ANIM_ASSERT(0);
		return 0;
	}

	uint32_t FindPosition(float AnimationTime, const AnimationChannel* channel, uint32_t& cursor)
	{
        return FindIndex(AnimationTime, channel->PositionKeys, cursor);
	}

	uint32_t FindScaling(float AnimationTime, const AnimationChannel* channel, uint32_t& cursor)
	{
        return FindIndex(AnimationTime, channel->ScalingKeys, cursor);
	}

	uint32_t FindRotation(float AnimationTime, const AnimationChannel* channel, uint32_t& cursor)
	{
        return FindIndex(AnimationTime, channel->RotationKeys, cursor);
	}

	XMVECTOR InterpolatePosition(float animTime, const AnimationChannel* channel, uint32_t& cursor)
	{
		auto outFloat3 = XMFLOAT3();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->PositionKeys.size() == 1) {
			Out = XMLoadFloat3(&channel->PositionKeys[0].Value);
			return Out;
		}

		uint32_t PositionIndex = FindPosition(animTime, channel, cursor);
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


	XMVECTOR InterpolateScaling(float animTime, const AnimationChannel* channel, uint32_t& cursor)
	{
		auto outFloat3 = XMFLOAT3();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->ScalingKeys.size() == 1) {
			Out = XMLoadFloat3(&channel->ScalingKeys[0].Value);
			return Out;
		}

		uint32_t ScaleIndex = FindScaling(animTime, channel, cursor);
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

	XMFLOAT4 InterpolateRotation(float animTime, const AnimationChannel* channel, uint32_t& cursor)
	{
		auto outFloat4 = XMFLOAT4();
		auto Out = XMVectorSet(0, 0, 0, 0);
		if (channel->RotationKeys.size() == 1) {
			//Out = XMLoadFloat4(&channel->RotationKeys[0].Value);
			return channel->RotationKeys[0].Value;
		}

		uint32_t RotationIndex = FindRotation(animTime, channel, cursor);
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