#include<iostream>
#include "FBXExporter.h"

int main(int argc, char** argv)
{
	FBXExporter* myExporter = new FBXExporter();
	myExporter->Initialize();
	myExporter->LoadScene(argv[1]);
	myExporter->ExportFBX();
	getchar();
	return 0;
}