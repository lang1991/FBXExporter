#pragma once
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Vertex.h"

struct CtrlpointWeightPair
{
	unsigned mCtrlPointIndex;
	double mWeight;
};

struct Keyframe
{
	FbxLongLong mFrameNum;
	FbxAMatrix mLocalTransform;
	Keyframe* mNext;

	Keyframe() :
		mNext(nullptr)
	{}
};

struct BoneInfoContainer
{
	FbxAMatrix mBindpose;
	vector<CtrlpointWeightPair> mBlendingInfo;
	FbxNode* mClusterLink;
	Keyframe* mAnimation;
	
	BoneInfoContainer():
		mClusterLink(nullptr),
		mAnimation(nullptr)
	{}
};

struct Bone
{
	string mName;
	int mParentIndex;
	FbxAMatrix mBindPose;
	Keyframe* mAnimation;
	FbxNode* mNode;

	Bone():
		mNode(nullptr),
		mAnimation(nullptr)
	{
		mBindPose.SetIdentity();
		mParentIndex = -1;
	}
};

struct Skeleton
{
	vector<Bone> mBones;
};

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();
	bool Initialize();
	bool LoadScene(const char* inFileName);
	void ProcessGeometry(FbxNode* inNode);
	void ProcessMesh(FbxNode* inNode);
	void ProcessBones(FbxNode* inNode);
	void ProcessSkeletonHierarchy(FbxNode* inRootNode);
	void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex);
	void ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, XMFLOAT3& outPosition);
	void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2& outUV);
	void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
	void ReadBinormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outBinormal);
	void ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outTangent);
	void AssociateBonesWithVertices();
	void CleanupFbxManager();
	
	void WriteMeshToStream(std::ostream& inStream);
	void WriteAnimationToStream(std::ostream& inStream);
	void ExportFBX();

private:
	FbxManager* mFBXManager;
	FbxScene* mFBXScene;
	float* mPositions;
	float* mNormals;
	float* mUVs;
	bool mHasNormal;
	bool mHasUV;
	bool mAllByControlPoint;
	unsigned int* mIndexBuffer;
	unsigned int mTriangleCount;
	std::vector<Vertex::PNTIWVertex> mVertices;
	Skeleton mSkeleton;
	unordered_map<string, BoneInfoContainer> mBoneInfoLookup;
	FbxLongLong mAnimationLength;
	string mAnimationName;

private:
	//void AssembleVertices();
	//int FindVertex(const Vertex::PNTVertex& inTarget, const std::vector<Vertex::PNTVertex>& inVertices);
	//void ReduceVertices();
	void PrintVertexBlendingInfo();
	FbxAMatrix GetGeometryTransformation(FbxNode* inNode);
	void TestGeometryTrans(FbxNode* inNode);
	void WriteMatrix(std::ostream& inStream, FbxAMatrix& inMatrix, bool inIsRoot);
	void WriteMatrix(std::ostream& inStream, FbxMatrix& inMatrix, bool inIsRoot);
	void PrintMatrix(FbxAMatrix& inMatrix);
	void PrintMatrix(FbxMatrix& inMatrix);

};