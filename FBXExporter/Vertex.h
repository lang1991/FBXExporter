#pragma once
#include <Windows.h>
#include <stdint.h>
#include <xnamath.h>

namespace Vertex
{
	struct PNTVertex
	{
		XMFLOAT3 mPosition;
		XMFLOAT3 mNormal;
		XMFLOAT2 mUV;

		bool operator==(const PNTVertex& rhs) const
		{
			uint32_t position;
			uint32_t normal;
			uint32_t uv;

			XMVectorEqualR(&position, XMLoadFloat3(&(this->mPosition)), XMLoadFloat3(&rhs.mPosition));
			XMVectorEqualR(&normal, XMLoadFloat3(&(this->mNormal)), XMLoadFloat3(&rhs.mNormal));
			XMVectorEqualR(&uv, XMLoadFloat2(&(this->mUV)), XMLoadFloat2(&rhs.mUV));

			return XMComparisonAllTrue(position) && XMComparisonAllTrue(normal) && XMComparisonAllTrue(uv);
		}
	};
}