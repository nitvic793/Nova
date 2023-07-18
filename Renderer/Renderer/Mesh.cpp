
#include "pch.h"
#include <Renderer/Mesh.h>

namespace nv::graphics
{
	void Mesh::CreateBoundingBox()
	{
		DirectX::BoundingBox meshBounds;
		DirectX::BoundingBox::CreateFromPoints(meshBounds, GetDesc().mVertices.size(), (const XMFLOAT3*)&GetDesc().mVertices.data()->mPosition, sizeof(Vertex));
		mBoundingBox.mBounding = meshBounds;
	}
}