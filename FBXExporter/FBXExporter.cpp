#include "FBXExporter.h"
#include <fstream>

FBXExporter::FBXExporter()
{
	m_FBXManager = nullptr;
	m_FBXScene = nullptr;
	m_IndexBuffer = nullptr;
	m_Positions = nullptr;
	m_Normals = nullptr;
	m_UVs = nullptr;
	mHasNormal = false;
	mHasUV = false;
	mAllByControlPoint = true;
	m_TriangleCount = 0;
}

bool FBXExporter::Initialize()
{
	m_FBXManager = FbxManager::Create();
	if (!m_FBXManager)
	{
		return false;
	}

	FbxIOSettings* fbxIOSettings = FbxIOSettings::Create(m_FBXManager, IOSROOT);
	m_FBXManager->SetIOSettings(fbxIOSettings);

	m_FBXScene = FbxScene::Create(m_FBXManager, "myScene");

	return true;
}

bool FBXExporter::LoadScene(const char* in_FileName)
{
	FbxImporter* fbxImporter = FbxImporter::Create(m_FBXManager, "myImporter");

	if (!fbxImporter)
	{
		return false;
	}

	if (!fbxImporter->Initialize(in_FileName, -1, m_FBXManager->GetIOSettings()))
	{
		return false;
	}

	if (!fbxImporter->Import(m_FBXScene))
	{
		return false;
	}
	fbxImporter->Destroy();

	return true;
}

void FBXExporter::ProcessNode(FbxNode* in_Node)
{
	if (in_Node->GetNodeAttribute())
	{
		switch (in_Node->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			ProcessMesh(in_Node);
			break;
		case FbxNodeAttribute::eSkeleton:
			break;
		}
	}

	for (int i = 0; i < in_Node->GetChildCount(); ++i)
	{
		FbxNode* temp = in_Node->GetChild(i);
		ProcessNode(in_Node->GetChild(i));
	}
}

void FBXExporter::ProcessMesh(FbxNode* in_Node)
{
	FbxMesh* pMesh = in_Node->GetMesh();
	const int lPolygonCount = pMesh->GetPolygonCount();
	m_TriangleCount = lPolygonCount;

	mHasNormal = pMesh->GetElementNormalCount() > 0;
	mHasUV = pMesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
	if (mHasNormal)
	{
		lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
		if (lNormalMappingMode == FbxGeometryElement::eNone)
		{
			mHasNormal = false;
		}
		if (mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}
	if (mHasUV)
	{
		lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
		if (lUVMappingMode == FbxGeometryElement::eNone)
		{
			mHasUV = false;
		}
		if (mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}

	int lPolygonVertexCount = pMesh->GetControlPointsCount();
	if (!mAllByControlPoint)
	{
		lPolygonVertexCount = lPolygonCount * 3;
	}
	float * lVertices = new float[lPolygonVertexCount * 4];
	unsigned int * lIndices = new unsigned int[lPolygonCount * 3];
	float * lNormals = NULL;
	if (mHasNormal)
	{
		lNormals = new float[lPolygonVertexCount * 3];
	}
	float * lUVs = NULL;
	FbxStringList lUVNames;
	pMesh->GetUVSetNames(lUVNames);
	const char * lUVName = NULL;
	if (mHasUV && lUVNames.GetCount())
	{
		lUVs = new float[lPolygonVertexCount * 2];
		lUVName = lUVNames[0];
	}

	const FbxVector4 * lControlPoints = pMesh->GetControlPoints();
	FbxVector4 lCurrentVertex;
	FbxVector4 lCurrentNormal;
	FbxVector2 lCurrentUV;
	if (mAllByControlPoint)
	{
		const FbxGeometryElementNormal * lNormalElement = NULL;
		const FbxGeometryElementUV * lUVElement = NULL;
		if (mHasNormal)
		{
			lNormalElement = pMesh->GetElementNormal(0);
		}
		if (mHasUV)
		{
			lUVElement = pMesh->GetElementUV(0);
		}
		for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
		{
			// Save the vertex position.
			lCurrentVertex = lControlPoints[lIndex];
			lVertices[lIndex * 4] = static_cast<float>(lCurrentVertex[0]);
			lVertices[lIndex * 4 + 1] = static_cast<float>(lCurrentVertex[1]);
			lVertices[lIndex * 4 + 2] = static_cast<float>(lCurrentVertex[2]);
			lVertices[lIndex * 4 + 3] = 1;

			// Save the normal.
			if (mHasNormal)
			{
				int lNormalIndex = lIndex;
				if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
				lNormals[lIndex * 3] = static_cast<float>(lCurrentNormal[0]);
				lNormals[lIndex * 3 + 1] = static_cast<float>(lCurrentNormal[1]);
				lNormals[lIndex * 3 + 2] = static_cast<float>(lCurrentNormal[2]);
			}

			// Save the UV.
			if (mHasUV)
			{
				int lUVIndex = lIndex;
				if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
				lUVs[lIndex * 2] = static_cast<float>(lCurrentUV[0]);
				lUVs[lIndex * 2 + 1] = static_cast<float>(lCurrentUV[1]);
			}
		}
	}

	int lVertexCount = 0;
	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	{
		for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
		{
			const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);

			// Populate the array with vertex attribute, if by polygon vertex.
			lIndices[lPolygonIndex * 3 + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

			lCurrentVertex = lControlPoints[lControlPointIndex];
			lVertices[lVertexCount * 4] = static_cast<float>(lCurrentVertex[0]);
			lVertices[lVertexCount * 4 + 1] = static_cast<float>(lCurrentVertex[1]);
			lVertices[lVertexCount * 4 + 2] = static_cast<float>(lCurrentVertex[2]);
			lVertices[lVertexCount * 4 + 3] = 1;

			if (mHasNormal)
			{
				pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
				lNormals[lVertexCount * 3] = static_cast<float>(lCurrentNormal[0]);
				lNormals[lVertexCount * 3 + 1] = static_cast<float>(lCurrentNormal[1]);
				lNormals[lVertexCount * 3 + 2] = static_cast<float>(lCurrentNormal[2]);
			}

			if (mHasUV)
			{
				bool lUnmappedUV;
				pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
				lUVs[lVertexCount * 2] = static_cast<float>(lCurrentUV[0]);
				lUVs[lVertexCount * 2 + 1] = static_cast<float>(lCurrentUV[1]);
			}
			++lVertexCount;
		}
	}

	m_Positions = lVertices;
	m_Normals = lNormals;
	m_UVs = lUVs;
	m_IndexBuffer = lIndices;
}

void FBXExporter::CleanupFbxManager()
{
	m_FBXScene->Destroy();
	m_FBXManager->Destroy();
}

void FBXExporter::ReduceVertices()
{
	CleanupFbxManager();
	std::vector<Vertex::PNTVertex> newVertices;
	for (unsigned int i = 0; i < m_Vertices.size(); ++i)
	{
		int index = FindVertex(m_Vertices[i], newVertices);
		if (index == -1)
		{
			m_IndexBuffer[i] = newVertices.size();
			newVertices.push_back(m_Vertices[i]);
		}
		else
		{
			m_IndexBuffer[i] = index;
		}
	}

	m_Vertices = newVertices;
}

void FBXExporter::WriteSceneToStream(std::ostream& IN_Stream)
{
	IN_Stream << "<?xml version='1.0' encoding='UTF-8' ?>" << std::endl;
	IN_Stream << "<itpmesh>" << std::endl;
	IN_Stream << "\t<format>pnt</format>" << std::endl;
	IN_Stream << "\t<texture>tex_Char_Roland_Body_Diff.tga</texture>" << std::endl;
	IN_Stream << "\t<triangles count='" << m_TriangleCount << "'>" << std::endl;
	for (unsigned int i = 0; i < m_TriangleCount; ++i)
	{
		// We need to change the culling order
		IN_Stream << "\t\t<tri>" << m_IndexBuffer[i * 3] << "," << m_IndexBuffer[i * 3 + 2] << "," << m_IndexBuffer[i * 3 + 1] << "</tri>" << std::endl;
	}
	IN_Stream << "\t</triangles>" << std::endl;

	IN_Stream << "\t<vertices count='" << m_Vertices.size() << "'>" << std::endl;
	for (unsigned int i = 0; i < m_Vertices.size(); ++i)
	{
		IN_Stream << "\t\t<vtx>" << std::endl;
		IN_Stream << "\t\t\t<pos>" << m_Vertices[i].mPosition.x << "," << m_Vertices[i].mPosition.y << "," << -m_Vertices[i].mPosition.z << "</pos>" << std::endl;
		IN_Stream << "\t\t\t<norm>" << m_Vertices[i].mNormal.x << "," << m_Vertices[i].mNormal.y << "," << -m_Vertices[i].mNormal.z << "</norm>" << std::endl;
		IN_Stream << "\t\t\t<tex>" << m_Vertices[i].mUV.x << "," << 1.0f - m_Vertices[i].mUV.y << "</tex>" << std::endl;
		IN_Stream << "\t\t</vtx>" << std::endl;
	}
	IN_Stream << "\t</vertices>" << std::endl;
	IN_Stream << "</itpmesh>" << std::endl;
}

void FBXExporter::ExportFBX()
{
	ProcessNode(m_FBXScene->GetRootNode());
	AssembleVertices();
	ReduceVertices();
	std::ofstream myfile(".\\exportedModels\\sonic.itpmesh");
	WriteSceneToStream(myfile);
	std::cout << "\n\nExport Done!\n";
}

void FBXExporter::AssembleVertices()
{
	for (unsigned int i = 0; i < m_TriangleCount * 3; ++i)
	{
		Vertex::PNTVertex tempVertex;
		tempVertex.mPosition.x = m_Positions[i * 4];
		tempVertex.mPosition.y = m_Positions[i * 4 + 1];
		tempVertex.mPosition.z = m_Positions[i * 4 + 2];
		tempVertex.mNormal.x = m_Normals[i * 3];
		tempVertex.mNormal.y = m_Normals[i * 3 + 1];
		tempVertex.mNormal.z = m_Normals[i * 3 + 2];
		tempVertex.mUV.x = m_UVs[i * 2];
		tempVertex.mUV.y = m_UVs[i * 2 + 1];
		m_Vertices.push_back(tempVertex);
	}

	delete[] m_Positions;
	delete[] m_Normals;
	delete[] m_UVs;
}

int FBXExporter::FindVertex(const Vertex::PNTVertex& IN_Target, const std::vector<Vertex::PNTVertex>& IN_Vertices)
{
	int index = -1;
	for (unsigned int i = 0; i < IN_Vertices.size(); ++i)
	{
		if (IN_Target == IN_Vertices[i])
		{
			index = i;
		}
	}

	return index;
}