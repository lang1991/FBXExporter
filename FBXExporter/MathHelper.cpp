#include "MathHelper.h"

const XMFLOAT2 MathHelper::vector2Epsilon = XMFLOAT2(0.001f, 0.001f);
const XMFLOAT2 MathHelper::vector2True = XMFLOAT2(0, 0);
const XMFLOAT3 MathHelper::vector3Epsilon = XMFLOAT3(0.001f, 0.001f, 0.001f);
const XMFLOAT3 MathHelper::vector3True = XMFLOAT3(0, 0, 0);


bool MathHelper::CompareVector3WithEpsilon(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
{
	uint32_t result;

	XMVectorEqualR(&result, XMVectorNearEqual(XMLoadFloat3(&lhs), XMLoadFloat3(&rhs), XMLoadFloat3(&vector3Epsilon)), XMLoadFloat3(&vector3True));
	return XMComparisonAllTrue(result);
}

bool MathHelper::CompareVector2WithEpsilon(const XMFLOAT2& lhs, const XMFLOAT2& rhs)
{
	uint32_t result;

	XMVectorEqualR(&result, XMVectorNearEqual(XMLoadFloat2(&lhs), XMLoadFloat2(&rhs), XMLoadFloat2(&vector2Epsilon)), XMLoadFloat2(&vector2True));
	return XMComparisonAllTrue(result);
}
