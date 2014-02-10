#pragma once
#include "Utilities.h"
#include <unordered_map>
#include "Material.h"

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();
	bool Initialize();
	bool LoadScene(const char* inFileName);
	
	void ExportFBX();

private:
	FbxManager* mFBXManager;
	FbxScene* mFBXScene;
	std::unordered_map<unsigned int, CtrlPoint*> mControlPoints; 
	unsigned int mTriangleCount;
	unsigned int* mIndexBuffer;
	std::vector<Triangle*> mTriangles;
	Skeleton mSkeleton;
	std::unordered_map<unsigned int, Material*> mMaterialLookUp;
	FbxLongLong mAnimationLength;
	std::string mAnimationName;
	
	

private:
	void ProcessGeometry(FbxNode* inNode);
	void ProcessSkeletonHierarchy(FbxNode* inRootNode);
	void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex);
	void ProcessControlPoints(FbxNode* inNode);
	void ProcessJointsAndAnimations(FbxNode* inNode);
	unsigned int FindJointIndexUsingName(const std::string& inJointName);
	void ProcessMesh(FbxNode* inNode);
	void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2& outUV);
	void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
	void ReadBinormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outBinormal);
	void ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outTangent);
	void Optimize();
	unsigned int FindVertex(PNTIWVertex* inTargetVertex);

	void AssociateMaterialToMesh(FbxNode* inNode);
	void ProcessMaterials(FbxNode* inNode);
	void ProcessMaterialAttribute(FbxSurfaceMaterial* inMaterial, unsigned int inMaterialIndex);
	void ProcessMaterialTexture(FbxSurfaceMaterial* inMaterial, Material* ioMaterial);
	void PrintMaterial();

	
	void CleanupFbxManager();
	void WriteMeshToStream(std::ostream& inStream);
	void WriteAnimationToStream(std::ostream& inStream);
	//int FindVertex(const Vertex::PNTVertex& inTarget, const std::vector<Vertex::PNTVertex>& inVertices);
	//void ReduceVertices();
	
};