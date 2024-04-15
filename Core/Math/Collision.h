#pragma once

#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <Engine/Component.h>

namespace nv::math
{
	struct BoundingBox : public ecs::IComponent
	{
		DirectX::BoundingBox mBounding;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive((math::float3)mBounding.Center);
			archive((math::float3)mBounding.Extents);
		}
	};

	struct BoundingSphere : public ecs::IComponent
	{
		DirectX::BoundingSphere mBounding;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive((math::float3)mBounding.Center);
			archive(mBounding.Radius);
		}
	};
}