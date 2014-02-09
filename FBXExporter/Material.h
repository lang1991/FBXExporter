#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "Vertex.h"

class Material
{
public:
	string mName;
	XMFLOAT3 mAmbient;
	XMFLOAT3 mDiffuse;
	XMFLOAT3 mEmissive;
	double mTransparencyFactor;
	string mDiffuseMapName;
	string mEmissiveMapName;
	string mGlossMapName;
	string mNormalMapName;
	string mSpecularMapName;

	virtual void WriteToStream(ostream& inStream) = 0;
};

class LambertMaterial : public Material
{
public:

	void WriteToStream(ostream& inStream)
	{
		inStream << "Ambient: " << mAmbient.x << " " << mAmbient.y << " " << mAmbient.z << endl;
		inStream << "Diffuse: " << mDiffuse.x << " " << mDiffuse.y << " " << mDiffuse.z << endl;
		inStream << "Emissive: " << mEmissive.x << " " << mEmissive.y << " " << mEmissive.z << endl;

		if (!mDiffuseMapName.empty())
		{
			inStream << "DiffuseMap: " << mDiffuseMapName << endl;
		}

		if (!mNormalMapName.empty())
		{
			inStream << "NormalMap: " << mNormalMapName << endl;
		}
	}
};

class PhongMaterial : public Material
{
public:
	XMFLOAT3 mSpecular;
	XMFLOAT3 mReflection;
	double mSpecularPower;
	double mShininess;
	double mReflectionFactor;

	void WriteToStream(ostream& inStream)
	{
		inStream << "Ambient: " << mAmbient.x << " " << mAmbient.y << " " << mAmbient.z << endl;
		inStream << "Diffuse: " << mDiffuse.x << " " << mDiffuse.y << " " << mDiffuse.z << endl;
		inStream << "Emissive: " << mEmissive.x << " " << mEmissive.y << " " << mEmissive.z << endl;
		inStream << "Specular: " << mSpecular.x << " " << mSpecular.y << " " << mSpecular.z << endl;
		inStream << "SpecPower: " << mSpecularPower << endl;
		inStream << "Reflectivity: " << mReflection.x << " " << mReflection.y << " " << mReflection.z <<endl;
		
		if (!mDiffuseMapName.empty())
		{
			inStream << "DiffuseMap: " << mDiffuseMapName << endl;
		}

		if(!mNormalMapName.empty())
		{
			inStream << "NormalMap: " << mNormalMapName << endl;
		}
	}
};