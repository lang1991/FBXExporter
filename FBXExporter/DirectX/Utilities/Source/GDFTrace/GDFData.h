//--------------------------------------------------------------------------------------
// File: GDFData.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#define MAX_DESCRIPTOR 128
#define MAX_GAMES 32

struct GDFRatingData
{
    WCHAR strRatingSystemGUID[256];
    WCHAR strRatingSystem[256];
    WCHAR strRatingIDGUID[256];
    WCHAR strRatingID[256];
    WCHAR strDescriptorGUID[MAX_DESCRIPTOR][256];
    WCHAR strDescriptor[MAX_DESCRIPTOR][256];
};

struct GDFData
{
    WORD wLanguage;
    WCHAR strLanguage[256];
    WCHAR strValidation[512];

    GDFRatingData ratingData[16];

    WCHAR strGameID[256];
    WCHAR strName[512];
    WCHAR strDescription[1025];
    WCHAR strReleaseDate[256];
    WCHAR strGenre[256];
    WCHAR strVersion[256];
    WCHAR strSavedGameFolder[256];
    float fSPRMin;
    float fSPRRecommended;
    WCHAR strDeveloper[256];
    WCHAR strDeveloperLink[256];
    WCHAR strPublisher[256];
    WCHAR strPublisherLink[256];
    WCHAR strType[256];
    WCHAR strRSS[2083];
    BOOL  fV2GDF;

    WCHAR strExe[MAX_GAMES][512];
};


HRESULT GetGDFDataFromGDF( GDFData* pGDFData, WCHAR* strGDFPath);
HRESULT GetGDFDataFromBIN( GDFData* pGDFData, WCHAR* strGDFBinPath, WORD wLanguage );

