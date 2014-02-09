#pragma once
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include <string>
#include "Vertex.h"
using namespace std;


struct BlendingIndexWeightPair
{
	unsigned int mBlendingIndex;
	double mBlendingWeight;

	BlendingIndexWeightPair() : 
		mBlendingIndex(0),
		mBlendingWeight(0)
	{}
};

// Each Control Point in FBX is basically a vertex
// in the physical world. For example, a cube has 8
// vertices(Control Points) in FBX
// Joints are associated with Control Points in FBX
// The mapping is one joint corresponding to 4
// Control Points(Reverse of what is done in a game engine)
// As a result, this struct stores a XMFLOAT3 and a 
// vector of joint indices
struct CtrlPoint
{
	XMFLOAT3 mPosition;
	vector<BlendingIndexWeightPair> mBlendingInfo;

	CtrlPoint()
	{
		mBlendingInfo.reserve(4);
	}
};

// This stores the information of each key frame of each joint
// This is a linked list and each node is a snapshot of the
// global transformation of the joint at a certain frame
struct Keyframe
{
	FbxLongLong mFrameNum;
	FbxAMatrix mGlobalTransform;
	Keyframe* mNext;

	Keyframe() :
		mNext(nullptr)
	{}
};

// This is the actual representation of a joint in a game engine
struct Joint
{
	string mName;
	int mParentIndex;
	FbxAMatrix mGlobalBindposeInverse;
	Keyframe* mAnimation;
	FbxNode* mNode;

	Joint() :
		mNode(nullptr),
		mAnimation(nullptr)
	{
		mGlobalBindposeInverse.SetIdentity();
		mParentIndex = -1;
	}
};

struct Skeleton
{
	vector<Joint> mJoints;
};

struct Triangle
{
	vector<PNTIWVertex*> mVertices;
	string mMaterialName;
	unsigned int mMaterialIndex;
};


class Utilities
{
public:

	static const XMFLOAT3 vector3Epsilon;
	static const XMFLOAT2 vector2Epsilon;
	static const XMFLOAT3 vector3True;
	static const XMFLOAT2 vector2True;


	// This function should be changed if exporting to another format
	static void WriteMatrix(std::ostream& inStream, FbxMatrix& inMatrix, bool inIsRoot)
	{
		inStream << "<mat>" << static_cast<float>(inMatrix.Get(0, 0)) << "," << static_cast<float>(inMatrix.Get(0, 1)) << "," << static_cast<float>(inMatrix.Get(0, 2)) << "," << static_cast<float>(inMatrix.Get(0, 3)) << ","
			<< static_cast<float>(inMatrix.Get(1, 0)) << "," << static_cast<float>(inMatrix.Get(1, 1)) << "," << static_cast<float>(inMatrix.Get(1, 2)) << "," << static_cast<float>(inMatrix.Get(1, 3)) << ","
			<< static_cast<float>(inMatrix.Get(2, 0)) << "," << static_cast<float>(inMatrix.Get(2, 1)) << "," << static_cast<float>(inMatrix.Get(2, 2)) << "," << static_cast<float>(inMatrix.Get(2, 3)) << ","
			<< static_cast<float>(inMatrix.Get(3, 0)) << "," << static_cast<float>(inMatrix.Get(3, 1)) << "," << static_cast<float>(inMatrix.Get(3, 2)) << "," << static_cast<float>(inMatrix.Get(3, 3)) << "</mat>\n";
	}

	static void PrintMatrix(FbxMatrix& inMatrix)
	{
		FbxString lMatrixValue;
		for (int k = 0; k<4; ++k)
		{
			FbxVector4 lRow = inMatrix.GetRow(k);
			char        lRowValue[1024];

			FBXSDK_sprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n", lRow[0], lRow[1], lRow[2], lRow[3]);
			lMatrixValue += FbxString("        ") + FbxString(lRowValue);
		}

		cout << lMatrixValue.Buffer();
	}

	static FbxAMatrix GetGeometryTransformation(FbxNode* inNode)
	{
		if (!inNode)
		{
			throw exception("Null for mesh geometry");
		}

		const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(lT, lR, lS);
	}

	static bool CompareVector3WithEpsilon(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
	{
		uint32_t result;

		XMVectorEqualR(&result, XMVectorNearEqual(XMLoadFloat3(&lhs), XMLoadFloat3(&rhs), XMLoadFloat3(&vector3Epsilon)), XMLoadFloat3(&vector3True));
		return XMComparisonAllTrue(result);
	}

	static bool CompareVector2WithEpsilon(const XMFLOAT2& lhs, const XMFLOAT2& rhs)
	{
		uint32_t result;

		XMVectorEqualR(&result, XMVectorNearEqual(XMLoadFloat2(&lhs), XMLoadFloat2(&rhs), XMLoadFloat2(&vector2Epsilon)), XMLoadFloat2(&vector2True));
		return XMComparisonAllTrue(result);
	}
};
