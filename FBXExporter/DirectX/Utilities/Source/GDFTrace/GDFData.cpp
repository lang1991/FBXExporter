//--------------------------------------------------------------------------------------
// File: GDFData.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#include <windows.h>
#include <rpcsal.h>
#include <gameux.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <assert.h>
#include <shellapi.h>
#include <shlobj.h>

#include "GDFData.h"
#include "GDFParse.h"
#include "RatingsDB.h"

WCHAR* ConvertLangIDtoString( WORD wLang );
HRESULT GetGDFData( GDFData* pGDFData, CGDFParse& gdfParse );

//--------------------------------------------------------------------------------------
HRESULT GetGDFDataFromGDF( GDFData* pGDFData, WCHAR* strGDFPath)
{
    CGDFParse gdfParse;
    gdfParse.ValidateGDF( strGDFPath, pGDFData->strValidation, 512 );

    HRESULT hr = gdfParse.LoadXML( strGDFPath );
    if ( FAILED(hr) )
        return E_FAIL;

    pGDFData->wLanguage = LANG_NEUTRAL;
    StringCchCopy( pGDFData->strLanguage, 256, L"LANG_UNKNOWN" );

    return GetGDFData( pGDFData, gdfParse);
}

//--------------------------------------------------------------------------------------
HRESULT GetGDFDataFromBIN( GDFData* pGDFData, WCHAR* strGDFBinPath, WORD wLanguage )
{
    CGDFParse gdfParse;
    gdfParse.ValidateXML( strGDFBinPath, wLanguage, pGDFData->strValidation, 512 );

    gdfParse.ExtractXML( strGDFBinPath, wLanguage );
    pGDFData->wLanguage = wLanguage;
    StringCchCopy( pGDFData->strLanguage, 256, ConvertLangIDtoString(wLanguage) );

    return GetGDFData( pGDFData, gdfParse);
}

//--------------------------------------------------------------------------------------
HRESULT GetGDFData( GDFData* pGDFData, CGDFParse& gdfParse )
{
    CRatingsDB ratingsDB;
    ratingsDB.LoadDB();
    
    gdfParse.GetGameID( pGDFData->strGameID, 256 );
    gdfParse.GetName( pGDFData->strName, 512 );
    gdfParse.GetDescription( pGDFData->strDescription, 1024 );
    gdfParse.GetReleaseDate( pGDFData->strReleaseDate, 256 );
    gdfParse.GetGenre( pGDFData->strGenre, 256 );
    gdfParse.GetVersion( pGDFData->strVersion, 256 );
    gdfParse.GetSavedGameFolder( pGDFData->strSavedGameFolder, 256 );
    gdfParse.GetWinSPR( &pGDFData->fSPRMin, &pGDFData->fSPRRecommended );
    gdfParse.GetDeveloper( pGDFData->strDeveloper, 256 );
    gdfParse.GetDeveloperLink( pGDFData->strDeveloperLink, 256 );
    gdfParse.GetPublisher( pGDFData->strPublisher, 256 );
    gdfParse.GetPublisherLink( pGDFData->strPublisherLink, 256 );
    gdfParse.GetType( pGDFData->strType, 256 );
    gdfParse.GetRSS( pGDFData->strRSS, 2083 );
    gdfParse.IsV2GDF( &pGDFData->fV2GDF );

    HRESULT hr;
    for( int iRating=0; iRating<16; iRating++ )
    {
        GDFRatingData* pGDFRatingData = &pGDFData->ratingData[iRating];
        hr = gdfParse.GetRatingSystem( iRating, pGDFRatingData->strRatingSystemGUID, 256 );
        if( FAILED(hr) )
            break;
        gdfParse.GetRatingID( iRating, pGDFRatingData->strRatingIDGUID, 256 );

        ratingsDB.GetRatingSystemName( pGDFRatingData->strRatingSystemGUID, pGDFRatingData->strRatingSystem, 256 );
        ratingsDB.GetRatingIDName( pGDFRatingData->strRatingSystemGUID, pGDFRatingData->strRatingIDGUID, pGDFRatingData->strRatingID, 256 );

        for( int iDescriptor=0; iDescriptor<MAX_DESCRIPTOR; iDescriptor++ )
        {
            hr = gdfParse.GetRatingDescriptor( iRating, iDescriptor, pGDFRatingData->strDescriptorGUID[iDescriptor], 256 );
            if( FAILED(hr) )
                break;
            ratingsDB.GetDescriptorName( pGDFRatingData->strRatingSystemGUID, pGDFRatingData->strDescriptorGUID[iDescriptor], pGDFRatingData->strDescriptor[iDescriptor], 256 );
        }
    }

    for( int iGame=0; iGame<MAX_GAMES; iGame++ )
    {
        hr = gdfParse.GetGameExe( iGame, pGDFData->strExe[iGame], 512 );
        if( FAILED(hr) )
            break;
    }

    return S_OK;
}



//--------------------------------------------------------------------------------------
#define CONV_LANGID(x) case x: return L#x;
WCHAR* ConvertLangIDtoString( WORD wLang )
{
    switch( LOBYTE(wLang) )
    {
        CONV_LANGID(LANG_NEUTRAL);
        CONV_LANGID(LANG_INVARIANT);
        CONV_LANGID(LANG_AFRIKAANS);
        CONV_LANGID(LANG_ARABIC);
        CONV_LANGID(LANG_ARMENIAN);
        CONV_LANGID(LANG_ASSAMESE);
        CONV_LANGID(LANG_AZERI);
        CONV_LANGID(LANG_BASQUE);
        CONV_LANGID(LANG_BELARUSIAN);
        CONV_LANGID(LANG_BENGALI);
        CONV_LANGID(LANG_BULGARIAN);
        CONV_LANGID(LANG_CATALAN);
        CONV_LANGID(LANG_CHINESE);
        CONV_LANGID(LANG_CZECH);
        CONV_LANGID(LANG_DANISH);
        CONV_LANGID(LANG_DIVEHI);
        CONV_LANGID(LANG_DUTCH);
        CONV_LANGID(LANG_ENGLISH);
        CONV_LANGID(LANG_ESTONIAN);
        CONV_LANGID(LANG_FAEROESE);
        CONV_LANGID(LANG_FINNISH);
        CONV_LANGID(LANG_FRENCH);
        CONV_LANGID(LANG_GALICIAN);
        CONV_LANGID(LANG_GEORGIAN);
        CONV_LANGID(LANG_GERMAN);
        CONV_LANGID(LANG_GREEK);
        CONV_LANGID(LANG_GUJARATI);
        CONV_LANGID(LANG_HEBREW);
        CONV_LANGID(LANG_HINDI);
        CONV_LANGID(LANG_HUNGARIAN);
        CONV_LANGID(LANG_ICELANDIC);
        CONV_LANGID(LANG_INDONESIAN);
        CONV_LANGID(LANG_ITALIAN);
        CONV_LANGID(LANG_JAPANESE);
        CONV_LANGID(LANG_KANNADA);
        CONV_LANGID(LANG_KAZAK);
        CONV_LANGID(LANG_KONKANI);
        CONV_LANGID(LANG_KOREAN);
        CONV_LANGID(LANG_KYRGYZ);
        CONV_LANGID(LANG_LATVIAN);
        CONV_LANGID(LANG_LITHUANIAN);
        CONV_LANGID(LANG_MACEDONIAN);
        CONV_LANGID(LANG_MALAY);
        CONV_LANGID(LANG_MALAYALAM);
        CONV_LANGID(LANG_MANIPURI);
        CONV_LANGID(LANG_MARATHI);
        CONV_LANGID(LANG_MONGOLIAN);
        CONV_LANGID(LANG_NEPALI);
        CONV_LANGID(LANG_NORWEGIAN);
        CONV_LANGID(LANG_ORIYA);
        CONV_LANGID(LANG_POLISH);
        CONV_LANGID(LANG_PORTUGUESE);
        CONV_LANGID(LANG_PUNJABI);
        CONV_LANGID(LANG_ROMANIAN);
        CONV_LANGID(LANG_RUSSIAN);
        CONV_LANGID(LANG_SANSKRIT);
        CONV_LANGID(LANG_SINDHI);
        CONV_LANGID(LANG_SLOVAK);
        CONV_LANGID(LANG_SLOVENIAN);
        CONV_LANGID(LANG_SPANISH);
        CONV_LANGID(LANG_SWAHILI);
        CONV_LANGID(LANG_SWEDISH);
        CONV_LANGID(LANG_SYRIAC);
        CONV_LANGID(LANG_TAMIL);
        CONV_LANGID(LANG_TATAR);
        CONV_LANGID(LANG_TELUGU);
        CONV_LANGID(LANG_THAI);
        CONV_LANGID(LANG_TURKISH);
        CONV_LANGID(LANG_UKRAINIAN);
        CONV_LANGID(LANG_URDU);
        CONV_LANGID(LANG_UZBEK);
        CONV_LANGID(LANG_VIETNAMESE);
    }
    return L"Unknown";
}


