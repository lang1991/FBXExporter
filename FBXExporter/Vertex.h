#pragma once
#include <Windows.h>
#include <stdint.h>
#include <xnamath.h>
#include <vector>
#include <algorithm>
#include "Utilities.h"
using namespace std;

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

struct VertexBlendingInfo
{
	unsigned int mBlendingIndex;
	double mBlendingWeight;

	VertexBlendingInfo():
		mBlendingIndex(0),
		mBlendingWeight(0.0)
	{}
};

struct PNTIWVertex
{
	XMFLOAT3 mPosition;
	XMFLOAT3 mNormal;
	XMFLOAT2 mUV;
	vector<VertexBlendingInfo> mVertexBlendingInfos;

	bool CompareBlendingInfos(const VertexBlendingInfo& info1, const VertexBlendingInfo& info2)
	{
		return (info1.mBlendingWeight < info2.mBlendingWeight);
	}

	void SortBlendingInfoByWeight()
	{
		sort(mVertexBlendingInfos.begin(), mVertexBlendingInfos.end(), CompareBlendingInfos);
	}

	bool operator==(const PNTIWVertex& rhs) const
	{
		bool sameBlendingInfo = true;

		// Each vertex should only have 4 index-weight blending info pairs
		for(unsigned int i = 0; i < 4; ++i)
		{
			if(mVertexBlendingInfos[i].mBlendingIndex != rhs.mVertexBlendingInfos[i].mBlendingIndex ||
				abs(mVertexBlendingInfos[i].mBlendingWeight - rhs.mVertexBlendingInfos[i].mBlendingWeight) > 0.001)
			{
				sameBlendingInfo = false;
				break;
			}
		}


		return Utilities::CompareVector3WithEpsilon(mPosition, rhs.mPosition) && 
			Utilities::CompareVector3WithEpsilon(mNormal, rhs.mNormal) &&
			Utilities::CompareVector2WithEpsilon(mUV, rhs.mUV) &&
			sameBlendingInfo;
	}
};