#pragma once
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include "Vertex.h"

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();
	bool Initialize();
	bool LoadScene(const char* inFileName);
	void ProcessNode(FbxNode* in_Node);
	void ProcessMesh(FbxNode* in_Node);
	void CleanupFbxManager();
	void ReduceVertices();
	void WriteSceneToStream(std::ostream& IN_Stream);
	void ExportFBX();

private:
	FbxManager* m_FBXManager;
	FbxScene* m_FBXScene;
	float* m_Positions;
	float* m_Normals;
	float* m_UVs;
	bool mHasNormal;
	bool mHasUV;
	bool mAllByControlPoint;
	unsigned int* m_IndexBuffer;
	unsigned int m_TriangleCount;
	std::vector<Vertex::PNTVertex> m_Vertices;
private:
	void AssembleVertices();
	int FindVertex(const Vertex::PNTVertex& IN_Target, const std::vector<Vertex::PNTVertex>& IN_Vertices);
};