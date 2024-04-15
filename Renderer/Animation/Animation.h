#pragma once

#include <Math/Math.h>
#include <unordered_map>
#include <string>
#include <Lib/Pool.h>
#include <AssetBase.h>
#include <Engine/Component.h>
#include <Renderer/Mesh.h>
#include <Interop/ShaderInteropTypes.h>
#include <Lib/ScopedPtr.h>

namespace nv::graphics::animation
{
	struct AnimationInstanceData
	{
		PerArmature			mArmatureConstantBuffer;
		BoneInfo			mBoneInfoList[MAX_BONES];
		uint32_t			mBoneInfoSize = 0;
	};

	struct VectorKey
	{
		double			Time;
		math::float3	Value;

		NV_SERIALIZE(Time, Value)
	};

	struct QuaternionKey
	{
		double			Time;
		math::float4	Value;

		NV_SERIALIZE(Time, Value)
	};

	struct AnimationChannel
	{
		std::string					NodeName;
		std::vector<VectorKey>		PositionKeys;
		std::vector<QuaternionKey>	RotationKeys;
		std::vector<VectorKey>		ScalingKeys;

		NV_SERIALIZE(NodeName, PositionKeys, RotationKeys, ScalingKeys)
	};

	struct Animation
	{
		std::string AnimationName;
		double TicksPerSecond;
		double Duration;
		std::vector<AnimationChannel> Channels;
		std::unordered_map<std::string, uint32_t> NodeChannelMap;

		NV_SERIALIZE(AnimationName, TicksPerSecond, Duration, Channels, NodeChannelMap)
	};

	struct MeshAnimNodeData
	{
		std::string RootNode;
		math::float4x4 GlobalInverseTransform;
		std::unordered_map<std::string, std::vector<std::string>> NodeHeirarchy;
		std::unordered_map<std::string, math::float4x4> NodeTransformsMap;

		NV_SERIALIZE(RootNode, GlobalInverseTransform, NodeHeirarchy, NodeTransformsMap)
	};

	struct AnimationStore
	{
		std::vector<Animation> Animations;
		std::unordered_map<std::string, uint32_t> AnimationIndexMap;

		NV_SERIALIZE(Animations, AnimationIndexMap)
	};

	uint32_t FindPosition(float AnimationTime, const AnimationChannel* channel);
	uint32_t FindScaling(float AnimationTime, const AnimationChannel* channel);
	uint32_t FindRotation(float AnimationTime, const AnimationChannel* channel);

	DirectX::XMVECTOR InterpolatePosition(float animTime, const AnimationChannel* channel);
	DirectX::XMVECTOR InterpolateScaling(float animTime, const AnimationChannel* channel);
	DirectX::XMFLOAT4 InterpolateRotation(float animTime, const AnimationChannel* channel);

	class AnimationManager
	{
	public:
		void Register(Handle<Mesh> meshHandle, const MeshAnimNodeData& data);
		void Register(uint64_t handle, Mesh* pMesh);
		void Register(const AnimationStore& store);

		AnimationInstanceData&		GetInstance(uint64_t handle);
		MeshAnimNodeData&			GetMeshAnimNodeData(Handle<Mesh> meshHandle);
		Animation*					GetAnimation(const std::string& animName);
		const Animation&			GetAnimation(uint32_t index) const;
		const std::string&			GetAnimationName(uint32_t index) const;
		int32_t						GetChannelIndex(uint32_t animationIndex, const std::string& node) const;
		const AnimationChannel*		GetChannel(uint32_t animIndex, const std::string& node) const;
		std::vector<std::string_view>	GetAnimationNames() const;
		uint32_t					GetAnimationCount() const;
		uint32_t					GetAnimationIndex(const char* pName) const;
		bool						IsAnimationIndexValid(uint32_t animationIndex) const;
		void						Lock();
		void						Unlock();

	private:
		std::unordered_map<uint64_t, ScopedPtr<AnimationInstanceData, true>> mAnimInstanceMap; // Maps to Handle<Entity>
		std::unordered_map<uint64_t, ScopedPtr<MeshAnimNodeData, true>> mMeshAnimNodeMap; // Maps to Handle<Mesh>
		AnimationStore mAnimStore;
	};

	extern AnimationManager gAnimManager;
}

