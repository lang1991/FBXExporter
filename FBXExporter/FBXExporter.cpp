#include "FBXExporter.h"
#include <fstream>
#include <sstream>
#include <iomanip>

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
	AssociateBonesWithVertices();
	std::ofstream meshOutput(".\\exportedModels\\simple2.itpmesh");
	std::ofstream animOutput(".\\exportedModels\\simple2.itpanim");
	WriteMeshToStream(meshOutput);
	WriteAnimationToStream(animOutput);
	std::cout << "\n\nExport Done!\n";
}

void FBXExporter::ProcessGeometry(FbxNode* inNode)
{
	if (inNode->GetNodeAttribute())
	{
		switch (inNode->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			ProcessMesh(inNode);
			ProcessBones(inNode);
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
			PNTIWVertex temp;
			temp.mPosition = position[j];
			temp.mNormal = normal[j];
			temp.mUV = UV[j][0];
			temp.mCtrlpointIndex = ctrlPointIndex;

			mVertices.push_back(temp);
			++vertexCounter;
		}
	}
}

void FBXExporter::ProcessBones(FbxNode* inNode)
{
	FbxMesh* currMesh = inNode->GetMesh();
	unsigned int numOfDeformers = currMesh->GetDeformerCount();
	FbxAMatrix geometryTransform = GetGeometryTransformation(inNode);
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
			FbxAMatrix transformMatrix;						// The transformation of the mesh at binding time
			FbxAMatrix transformLinkMatrix;					// The transformation of the cluster(joint) at binding time from joint space to world space
			FbxAMatrix localBindposeMatrix;
			currCluster->GetTransformMatrix(transformMatrix);
			currCluster->GetTransformLinkMatrix(transformLinkMatrix);
			localBindposeMatrix = transformLinkMatrix;/*.Inverse() * transformMatrix * geometryTransform;*/

			BoneInfoContainer currBoneInfo;
			currBoneInfo.mClusterLink = currCluster->GetLink();
			currBoneInfo.mBindpose = localBindposeMatrix;
			unsigned int numOfIndices = currCluster->GetControlPointIndicesCount();
			for(unsigned int i = 0; i < numOfIndices; ++i)
			{
				CtrlpointWeightPair currCtrlpointWeightPair;
				currCtrlpointWeightPair.mCtrlPointIndex = currCluster->GetControlPointIndices()[i];
				currCtrlpointWeightPair.mWeight = currCluster->GetControlPointWeights()[i];
				currBoneInfo.mBlendingInfo.push_back(currCtrlpointWeightPair);
			}


			// Now only supports one take
			FbxAnimStack* currAnimStack = mFBXScene->GetSrcObject<FbxAnimStack>(0);
			FbxString animStackName = currAnimStack->GetName();
			mAnimationName = animStackName.Buffer();
			FbxTakeInfo* takeInfo = mFBXScene->GetTakeInfo(animStackName);
			FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
			FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
			mAnimationLength = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;
			Keyframe** currAnim = &currBoneInfo.mAnimation;

			for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
			{
				FbxTime currTime;
				currTime.SetFrame(i, FbxTime::eFrames24);
				*currAnim = new Keyframe();
				(*currAnim)->mFrameNum = i;
				(*currAnim)->mLocalTransform = currCluster->GetLink()->EvaluateGlobalTransform(currTime);
				currAnim = &((*currAnim)->mNext);
			}

			mBoneInfoLookup[currCluster->GetLink()->GetName()] = currBoneInfo;
		}
	}

	for(unsigned int i = 0; i < mSkeleton.mBones.size(); ++i)
	{
		auto update = mBoneInfoLookup.find(mSkeleton.mBones[i].mName);
		if (update != mBoneInfoLookup.end())
		{
			mSkeleton.mBones[i].mBindPose = update->second.mBindpose;
			mSkeleton.mBones[i].mAnimation = update->second.mAnimation;
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

void FBXExporter::AssociateBonesWithVertices()
{
	// I will definitely rewrite this function...............................
	for (auto itr = mBoneInfoLookup.begin(); itr != mBoneInfoLookup.end(); ++itr)
	{
		for(unsigned int i = 0; i < itr->second.mBlendingInfo.size(); ++i)
		{
			for(unsigned int j = 0; j < mVertices.size(); ++j)
			{
				if (mVertices[j].mCtrlpointIndex == itr->second.mBlendingInfo[i].mCtrlPointIndex)
				{
					for(unsigned int k = 0; k < mSkeleton.mBones.size(); ++k)
					{
						if (mSkeleton.mBones[k].mName == itr->second.mClusterLink->GetName())
						{
							VertexBlendingInfo currBlendingInfo;
							currBlendingInfo.mBlendingIndex = k;
							currBlendingInfo.mBlendingWeight = static_cast<float>(itr->second.mBlendingInfo[i].mWeight);
							mVertices[j].mVertexBlendingInfos.push_back(currBlendingInfo);
							break;
						}
					}
				}
			}
		}
	}
	

	for (unsigned int i = 0; i < mVertices.size(); ++i)
	{
		if (mVertices[i].mVertexBlendingInfos.size() < 4)
		{
			VertexBlendingInfo currBlendingInfo;
			for (unsigned int j = 4 - mVertices[i].mVertexBlendingInfos.size(); j > 0; --j)
			{
				mVertices[i].mVertexBlendingInfos.push_back(currBlendingInfo);
			}
		}
	}
}

void FBXExporter::CleanupFbxManager()
{
	mFBXScene->Destroy();
	mFBXManager->Destroy();
}

/*
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
*/

void FBXExporter::WriteMeshToStream(std::ostream& inStream)
{
	inStream << "<?xml version='1.0' encoding='UTF-8' ?>" << std::endl;
	inStream << "<itpmesh>" << std::endl;
	inStream << "\t<!-- position, normal, skinning weights, skinning indices, texture-->" << std::endl;
	inStream << "\t<format>pnst</format>" << std::endl;
	inStream << "\t<texture>simple2.jpg</texture>" << std::endl;
	inStream << "\t<triangles count='" << mTriangleCount << "'>" << std::endl;
	for (unsigned int i = 0; i < mTriangleCount; ++i)
	{
		// We need to change the culling order
		//inStream << "\t\t<tri>" << mIndexBuffer[i * 3] << "," << mIndexBuffer[i * 3 + 2] << "," << mIndexBuffer[i * 3 + 1] << "</tri>" << std::endl;
		inStream << "\t\t<tri>" << mIndexBuffer[i * 3 + 2] << "," << mIndexBuffer[i * 3 + 1] << "," << mIndexBuffer[i * 3] << "</tri>" << std::endl;
	}
	inStream << "\t</triangles>" << std::endl;

	inStream << "\t<vertices count='" << mVertices.size() << "'>" << std::endl;
	for (unsigned int i = 0; i < mVertices.size(); ++i)
	{
		inStream << "\t\t<vtx>" << std::endl;
		inStream << "\t\t\t<pos>" << mVertices[i].mPosition.x << "," << mVertices[i].mPosition.y << "," << -mVertices[i].mPosition.z << "</pos>" << std::endl;
		inStream << "\t\t\t<norm>" << mVertices[i].mNormal.x << "," << mVertices[i].mNormal.y << "," << -mVertices[i].mNormal.z << "</norm>" << std::endl;
		inStream << "\t\t\t<sw>" << mVertices[i].mVertexBlendingInfos[0].mBlendingWeight << "," << mVertices[i].mVertexBlendingInfos[1].mBlendingWeight << "," << mVertices[i].mVertexBlendingInfos[2].mBlendingWeight << "," << mVertices[i].mVertexBlendingInfos[3].mBlendingWeight << "</sw>" << std::endl;
		inStream << "\t\t\t<si>" << mVertices[i].mVertexBlendingInfos[0].mBlendingIndex << "," << mVertices[i].mVertexBlendingInfos[1].mBlendingIndex << "," << mVertices[i].mVertexBlendingInfos[2].mBlendingIndex << "," << mVertices[i].mVertexBlendingInfos[3].mBlendingIndex << "</si>" << std::endl;
		//inStream << "\t\t\t<tex>" << mVertices[i].mUV.x << "," << 1.0f - mVertices[i].mUV.y << "</tex>" << std::endl;
		inStream << "\t\t\t<tex>" << mVertices[i].mUV.x << "," << -1.0f * mVertices[i].mUV.y << "</tex>" << std::endl;
		inStream << "\t\t</vtx>" << std::endl;
	}
	inStream << "\t</vertices>" << std::endl;
	inStream << "</itpmesh>" << std::endl;
}

void FBXExporter::WriteAnimationToStream(std::ostream& inStream)
{
	inStream << "<?xml version='1.0' encoding='UTF-8' ?>" << std::endl;
	inStream << "<itpanim>" << std::endl;
	inStream << "\t<skeleton count='" << mSkeleton.mBones.size() << "'>" <<std::endl;
	for(unsigned int i = 0; i < mSkeleton.mBones.size(); ++i)
	{
		inStream << "\t\t<joint id='" << i << "' name='" << mSkeleton.mBones[i].mName << "' parent='" << mSkeleton.mBones[i].mParentIndex <<"'>\n";
		inStream << "\t\t\t";
		if(true)//if(i == 0)
		{
			WriteMatrix(inStream, mSkeleton.mBones[i].mBindPose.Transpose(), true);
		}
		else
		{
			WriteMatrix(inStream, mSkeleton.mBones[i].mBindPose.Transpose(), false);
		}
		inStream << "\t\t</joint>\n";
	}
	inStream << "\t</skeleton>\n";
	inStream << "\t<animations>\n";
	inStream << "\t\t<animation name='" << mAnimationName << "' length='" << mAnimationLength << "'>\n";
	for(unsigned int i = 0; i < mSkeleton.mBones.size(); ++i)
	{
		inStream << "\t\t\t" << "<track id = '" << i << "' name='" << mSkeleton.mBones[i].mName << "'>\n";
		Keyframe* walker = mSkeleton.mBones[i].mAnimation;
		while(walker)
		{
			inStream << "\t\t\t\t" << "<frame num='" << walker->mFrameNum - 1 << "'>\n";
			inStream << "\t\t\t\t\t";
			if(true)//if(i == 0)
			{
				WriteMatrix(inStream, walker->mLocalTransform.Transpose(), true);
			}
			else
			{
				WriteMatrix(inStream, walker->mLocalTransform.Transpose(), false);
			}
			inStream << "\t\t\t\t" << "</frame>\n";
			walker = walker->mNext;
		}
		inStream << "\t\t\t" << "</track>\n";
	}
	inStream << "\t\t</animation>\n";
	inStream << "</animations>\n";
	inStream << "</itpanim>";
}

/*
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
*/

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

void FBXExporter::PrintVertexBlendingInfo()
{
	for(unsigned int i = 0; i < mVertices.size(); ++i)
	{
		cout<<"Vertex: "<<i<<"\n";
		for (unsigned int j = 0; j < mVertices[i].mVertexBlendingInfos.size(); ++j)
		{
			cout << "Info# " << j + 1 << ": " << "Index: " << mVertices[i].mVertexBlendingInfos[j].mBlendingIndex << " Weight: " << mVertices[i].mVertexBlendingInfos[j].mBlendingWeight;
			cout<<"\n";
		}

		cout<<"\n\n";
	}
}

void FBXExporter::WriteMatrix(std::ostream& inStream, FbxAMatrix& inMatrix, bool inIsRoot)
{
	if(!inIsRoot)
	{
		inStream << "<mat>" << static_cast<float>(inMatrix.Get(0, 0)) << "," << static_cast<float>(inMatrix.Get(0, 1)) << "," << static_cast<float>(inMatrix.Get(0, 2)) << "," << static_cast<float>(inMatrix.Get(0, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(1, 0)) << "," << static_cast<float>(inMatrix.Get(1, 1)) << "," << static_cast<float>(inMatrix.Get(1, 2)) << "," << static_cast<float>(inMatrix.Get(1, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(2, 0)) << "," << static_cast<float>(inMatrix.Get(2, 1)) << "," << static_cast<float>(inMatrix.Get(2, 2)) << "," << static_cast<float>(inMatrix.Get(2, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(3, 0)) << "," << static_cast<float>(inMatrix.Get(3, 1)) << "," << static_cast<float>(inMatrix.Get(3, 2)) << "," << static_cast<float>(inMatrix.Get(3, 3)) << "</mat>\n";
	}
	else
	{
		inStream << "<mat>" << static_cast<float>(inMatrix.Get(0, 0)) << "," << static_cast<float>(inMatrix.Get(0, 1)) << "," << static_cast<float>(-1 * inMatrix.Get(0, 2)) << "," << static_cast<float>(inMatrix.Get(0, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(1, 0)) << "," << static_cast<float>(inMatrix.Get(1, 1)) << "," << static_cast<float>(-1 * inMatrix.Get(1, 2)) << "," << static_cast<float>(inMatrix.Get(1, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(2, 0)) << "," << static_cast<float>(inMatrix.Get(2, 1)) << "," << static_cast<float>(-1 * inMatrix.Get(2, 2)) << "," << static_cast<float>(inMatrix.Get(2, 3)) << "," 
		<< static_cast<float>(inMatrix.Get(3, 0)) << "," << static_cast<float>(inMatrix.Get(3, 1)) << "," << static_cast<float>(-1 * inMatrix.Get(3, 2)) << "," << static_cast<float>(inMatrix.Get(3, 3)) << "</mat>\n";
	}
}