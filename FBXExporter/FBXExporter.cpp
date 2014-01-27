#include "FBXExporter.h"
#include <fstream>
#include <sstream>

using namespace Vertex;

FBXExporter::FBXExporter()
{
	mFBXManager = nullptr;
	mFBXScene = nullptr;
	mIndexBuffer = nullptr;
	mPositions = nullptr;
	mNormals = nullptr;
	mUVs = nullptr;
	mHasNormal = false;
	mHasUV = false;
	mAllByControlPoint = true;
	mTriangleCount = 0;
}

bool FBXExporter::Initialize()
{
	mFBXManager = FbxManager::Create();
	if (!mFBXManager)
	{
		return false;
	}

	FbxIOSettings* fbxIOSettings = FbxIOSettings::Create(mFBXManager, IOSROOT);
	mFBXManager->SetIOSettings(fbxIOSettings);

	mFBXScene = FbxScene::Create(mFBXManager, "myScene");

	return true;
}

bool FBXExporter::LoadScene(const char* inFileName)
{
	FbxImporter* fbxImporter = FbxImporter::Create(mFBXManager, "myImporter");

	if (!fbxImporter)
	{
		return false;
	}

	if (!fbxImporter->Initialize(inFileName, -1, mFBXManager->GetIOSettings()))
	{
		return false;
	}

	if (!fbxImporter->Import(mFBXScene))
	{
		return false;
	}
	fbxImporter->Destroy();

	return true;
}

void FBXExporter::ExportFBX()
{
	ProcessSkeletonHierarchy(mFBXScene->GetRootNode());
	ProcessGeometry(mFBXScene->GetRootNode());
	//AssembleVertices();
	//ReduceVertices();
	std::ofstream myfile(".\\exportedModels\\skybreakerIdle.itpmesh");
	WriteSceneToStream(myfile);
	std::cout << "\n\nExport Done!\n";
}

void FBXExporter::ProcessGeometry(FbxNode* inNode)
{
	if (inNode->GetNodeAttribute())
	{
		switch (inNode->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			//TestGeometryTrans(inNode);
			/*
			FbxMatrix convert;
			FbxMatrix temp(mFBXScene->GetAnimationEvaluator()->GetNodeGlobalTransform(inNode));
			convert.mData[0][0] = 1;
			convert.mData[0][1] = 0;
			convert.mData[0][2] = 0;
			convert.mData[0][3] = 0;
			convert.mData[1][0] = 0;
			convert.mData[1][1] = 0;
			convert.mData[1][2] = 1;
			convert.mData[1][3] = 0;
			convert.mData[2][0] = 0;
			convert.mData[2][1] = 1;
			convert.mData[2][2] = 0;
			convert.mData[2][3] = 0;
			convert.mData[3][0] = 0;
			convert.mData[3][1] = 0;
			convert.mData[3][2] = 0;
			convert.mData[3][3] = 1;
			*/
			//PrintMatrix(temp * convert);
			ProcessMesh(inNode);
			//ProcessBones(inNode);
			break;
		}
	}

	for (int i = 0; i < inNode->GetChildCount(); ++i)
	{
		ProcessGeometry(inNode->GetChild(i));
	}
}

void FBXExporter::ProcessMesh(FbxNode* inNode)
{
	FbxMesh* currMesh = inNode->GetMesh();
	if(!currMesh)
	{
		return;
	}

	int numTriangle = currMesh->GetPolygonCount();
	mTriangleCount = numTriangle;
	int vertexCounter = 0;

	mIndexBuffer = new unsigned int[numTriangle * 3];
	for(int i = 0; i < numTriangle; ++i)
	{
		XMFLOAT3 position[3];
		XMFLOAT3 normal[3];
		XMFLOAT3 tangent[3];
		XMFLOAT3 binormal[3];
		XMFLOAT2 UV[3][2];

		for(int j = 0; j < 3; ++j)
		{
			int ctrlPointIndex = currMesh->GetPolygonVertex(i, j);
			
			ReadPosition(currMesh, ctrlPointIndex, position[j]);
			ReadNormal(currMesh, ctrlPointIndex, vertexCounter, normal[j]);

			// We only have diffuse texture
			for(int k = 0; k < 1; ++k)
			{
				ReadUV(currMesh, ctrlPointIndex, currMesh->GetTextureUVIndex(i, j), k, UV[j][k]);
			}


			mIndexBuffer[i * 3 + j] = vertexCounter;
			PNTVertex temp;
			temp.mPosition = position[j];
			temp.mNormal = normal[j];
			temp.mUV = UV[j][0];

			mVertices.push_back(temp);
			++vertexCounter;
		}
	}
}

void FBXExporter::ProcessBones(FbxNode* inNode)
{
	FbxMesh* currMesh = inNode->GetMesh();
	unsigned int numOfDeformers = currMesh->GetDeformerCount();
	for (unsigned int deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
	{
		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(currMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if(!currSkin)
		{
			continue;
		}

		unsigned int numOfClusters = currSkin->GetClusterCount();
		for(unsigned int clusterIndex = 0; clusterIndex < numOfClusters; ++clusterIndex)
		{
			FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
			int count = 0;
			if (currCluster->GetAssociateModel())
			{
				cout << "This joint: " << currCluster->GetLink()->GetName()<<" is fucked up!\n";
				++count;
			}
			cout<<"# of unfucked joint: "<<count<<"\n";
			cout<<"# of total joint: "<<mSkeleton.mBones.size()<<"\n";
			/*
			FbxAMatrix transformLinkInv;
			currCluster->GetTransformLinkMatrix(transformLinkInv);
			transformLinkInv = transformLinkInv.Inverse();
			FbxAMatrix transform;
			currCluster->GetTransformMatrix(transform);
			FbxAMatrix bindposeInv = (transformLinkInv * transform).Inverse();
			mBindposeLookup[currCluster->GetLink()->GetName()] = bindposeInv;


			cout << "Name of bone: " << currCluster->GetLink()->GetName() << endl;
			cout<<"Inverse Bindpose:\n";
			
			FbxString lMatrixValue;
			for (int k = 0; k<4; ++k)
			{
				FbxVector4 lRow = transform.GetRow(k);
				char        lRowValue[1024];
				FBXSDK_sprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n", lRow[0], lRow[1], lRow[2], lRow[3]);
				lMatrixValue += FbxString("        ") + FbxString(lRowValue);
			}

			cout << lMatrixValue.Buffer();
			cout<<endl;
			*/
		}
		cout<<"\n\n\n";
	}

	for(unsigned int i = 0; i < mSkeleton.mBones.size(); ++i)
	{
		auto update = mBindposeLookup.find(mSkeleton.mBones[i].mName);
		if (update != mBindposeLookup.end())
		{
			mSkeleton.mBones[i].mBindPose = update->second;
		}
	}
}

void FBXExporter::ProcessSkeletonHierarchy(FbxNode* inRootNode)
{
	
	for(int childIndex = 0; childIndex < inRootNode->GetChildCount(); ++childIndex)
	{
		FbxNode* currNode = inRootNode->GetChild(childIndex);
		if (currNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			ProcessSkeletonHierarchyRecursively(currNode, 0, 0, -1);
		}
	}
}

void FBXExporter::ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex)
{
	/*
	FbxString lString;
	stringstream ss;
	int i;

	for (i = 0; i < inDepth; i++)
	{
		lString += "     ";
	}
	*/

	Bone currBone;
	currBone.mParentIndex = inParentIndex;
	currBone.mName = inNode->GetName();
	currBone.mFbxNode = inNode;
	mSkeleton.mBones.push_back(currBone);

	/*
	lString += inNode->GetName();
	lString += " | Index: ";
	ss << myIndex;
	lString.Append(ss.str().c_str(), ss.str().size());
	ss.str("");
	lString += " | Parent Index: ";
	ss << inParentIndex;
	lString.Append(ss.str().c_str(), ss.str().size());
	lString += "\n";
	FBXSDK_printf(lString.Buffer());
	*/

	for (int i = 0; i < inNode->GetChildCount(); i++)
	{
		ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), inDepth + 1, mSkeleton.mBones.size(), myIndex);
	}
}

void FBXExporter::ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, XMFLOAT3& outPosition)
{
	FbxVector4* ctrlPoints = inMesh->GetControlPoints();

	outPosition.x = static_cast<float>(ctrlPoints[inCtrlPointIndex].mData[0]);
	outPosition.y = static_cast<float>(ctrlPoints[inCtrlPointIndex].mData[1]);
	outPosition.z = static_cast<float>(ctrlPoints[inCtrlPointIndex].mData[2]);
}

void FBXExporter::ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2& outUV)
{
	if(inUVLayer >= 2 || inMesh->GetElementUVCount() <= inUVLayer)
	{
		throw std::exception("Invalid UV Layer Number");
	}
	FbxGeometryElementUV* vertexUV = inMesh->GetElementUV(inUVLayer);

	switch(vertexUV->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch(vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexUV->GetIndexArray().GetAt(inCtrlPointIndex);
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch(vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		case FbxGeometryElement::eIndexToDirect:
		{
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(inTextureUVIndex).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(inTextureUVIndex).mData[1]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;
	}
}

void FBXExporter::ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal)
{
	if(inMesh->GetElementNormalCount() < 1)
	{
		throw std::exception("Invalid Normal Number");
	}

	FbxGeometryElementNormal* vertexNormal = inMesh->GetElementNormal(0);
	switch(vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch(vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inCtrlPointIndex);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch(vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inVertexCounter);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;
	}
}

void FBXExporter::ReadBinormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outBinormal)
{
	if(inMesh->GetElementBinormalCount() < 1)
	{
		throw std::exception("Invalid Binormal Number");
	}

	FbxGeometryElementBinormal* vertexBinormal = inMesh->GetElementBinormal(0);
	switch(vertexBinormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch(vertexBinormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outBinormal.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outBinormal.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
			outBinormal.z = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexBinormal->GetIndexArray().GetAt(inCtrlPointIndex);
			outBinormal.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[0]);
			outBinormal.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[1]);
			outBinormal.z = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch(vertexBinormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outBinormal.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outBinormal.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
			outBinormal.z = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexBinormal->GetIndexArray().GetAt(inVertexCounter);
			outBinormal.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[0]);
			outBinormal.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[1]);
			outBinormal.z = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;
	}
}

void FBXExporter::ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outTangent)
{
	if(inMesh->GetElementTangentCount() < 1)
	{
		throw std::exception("Invalid Tangent Number");
	}

	FbxGeometryElementTangent* vertexTangent = inMesh->GetElementTangent(0);
	switch(vertexTangent->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch(vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(inCtrlPointIndex);
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch(vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[1]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(inVertexCounter);
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;
	}
}

void FBXExporter::CleanupFbxManager()
{
	mFBXScene->Destroy();
	mFBXManager->Destroy();
}

void FBXExporter::ReduceVertices()
{
	CleanupFbxManager();
	std::vector<Vertex::PNTVertex> newVertices;
	for (unsigned int i = 0; i < mVertices.size(); ++i)
	{
		int index = FindVertex(mVertices[i], newVertices);
		if (index == -1)
		{
			mIndexBuffer[i] = newVertices.size();
			newVertices.push_back(mVertices[i]);
		}
		else
		{
			mIndexBuffer[i] = index;
		}
	}

	mVertices = newVertices;
}

void FBXExporter::WriteSceneToStream(std::ostream& inStream)
{
	inStream << "<?xml version='1.0' encoding='UTF-8' ?>" << std::endl;
	inStream << "<itpmesh>" << std::endl;
	inStream << "\t<format>pnt</format>" << std::endl;
	inStream << "\t<texture>skybreaker_diff.tga</texture>" << std::endl;
	inStream << "\t<triangles count='" << mTriangleCount << "'>" << std::endl;
	for (unsigned int i = 0; i < mTriangleCount; ++i)
	{
		// We need to change the culling order
		inStream << "\t\t<tri>" << mIndexBuffer[i * 3] << "," << mIndexBuffer[i * 3 + 2] << "," << mIndexBuffer[i * 3 + 1] << "</tri>" << std::endl;
	}
	inStream << "\t</triangles>" << std::endl;

	inStream << "\t<vertices count='" << mVertices.size() << "'>" << std::endl;
	for (unsigned int i = 0; i < mVertices.size(); ++i)
	{
		inStream << "\t\t<vtx>" << std::endl;
		inStream << "\t\t\t<pos>" << mVertices[i].mPosition.x << "," << mVertices[i].mPosition.y << "," << -mVertices[i].mPosition.z << "</pos>" << std::endl;
		inStream << "\t\t\t<norm>" << mVertices[i].mNormal.x << "," << mVertices[i].mNormal.y << "," << -mVertices[i].mNormal.z << "</norm>" << std::endl;
		inStream << "\t\t\t<tex>" << mVertices[i].mUV.x << "," << 1.0f - mVertices[i].mUV.y << "</tex>" << std::endl;
		inStream << "\t\t</vtx>" << std::endl;
	}
	inStream << "\t</vertices>" << std::endl;
	inStream << "</itpmesh>" << std::endl;
}

void FBXExporter::AssembleVertices()
{
	for (unsigned int i = 0; i < mTriangleCount * 3; ++i)
	{
		Vertex::PNTVertex tempVertex;
		tempVertex.mPosition.x = mPositions[i * 4];
		tempVertex.mPosition.y = mPositions[i * 4 + 1];
		tempVertex.mPosition.z = mPositions[i * 4 + 2];
		tempVertex.mNormal.x = mNormals[i * 3];
		tempVertex.mNormal.y = mNormals[i * 3 + 1];
		tempVertex.mNormal.z = mNormals[i * 3 + 2];
		tempVertex.mUV.x = mUVs[i * 2];
		tempVertex.mUV.y = mUVs[i * 2 + 1];
		mVertices.push_back(tempVertex);
	}

	delete[] mPositions;
	delete[] mNormals;
	delete[] mUVs;
}

int FBXExporter::FindVertex(const Vertex::PNTVertex& inTarget, const std::vector<Vertex::PNTVertex>& inVertices)
{
	int index = -1;
	for (unsigned int i = 0; i < inVertices.size(); ++i)
	{
		if (inTarget == inVertices[i])
		{
			index = i;
		}
	}

	return index;
}

FbxAMatrix FBXExporter::GetGeometryTransformation(FbxNode* inNode)
{
	if(!inNode)
	{
		throw exception("Null for mesh geometry");
	}

	const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void FBXExporter::TestGeometryTrans(FbxNode* inNode)
{
	FbxAMatrix geometryTrans = GetGeometryTransformation(inNode);
	PrintMatrix(geometryTrans);
}

void FBXExporter::PrintMatrix(FbxAMatrix& inMatrix)
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

void FBXExporter::PrintMatrix(FbxMatrix& inMatrix)
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