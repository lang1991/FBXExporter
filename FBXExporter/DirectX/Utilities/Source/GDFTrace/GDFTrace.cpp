//-----------------------------------------------------------------------------
// File: GDFTrace.cpp
//
// Desc: Windows code that calls GameuxInstallHelper sample dll and displays the results.
//
// (C) Copyright Microsoft Corp.  All rights reserved.
//-----------------------------------------------------------------------------
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
#include <rpcsal.h>
#include <gameux.h>
#include <shellapi.h>
#include <strsafe.h>
#include <shlobj.h>
#include <wbemidl.h>
#include <objbase.h>
#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include "GDFParse.h"
#include "RatingsDB.h"
#include "GDFData.h"
#include "d3dx10.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

struct SETTINGS
{
    WCHAR strGDFBinPath[MAX_PATH];
    bool bGDFInsteadOfBinary;
    bool bMuteGDF;
    bool bMuteWarnings;
    bool bQuiet;    
};

SETTINGS g_settings;
int g_nNumberOfWarnings = 0;

bool ParseCommandLine( SETTINGS* pSettings );
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg );
void DisplayUsage();

struct SValue
{
    LPWSTR pName;
    DWORD dwValue;
};

#define DEFFMT(fmt) { L# fmt, DXGI_FORMAT_ ## fmt }

SValue g_pD3DX10Formats[] = 
{
    DEFFMT(R32G32B32A32_TYPELESS),
	DEFFMT(R32G32B32A32_FLOAT),
	DEFFMT(R32G32B32A32_UINT),
	DEFFMT(R32G32B32A32_SINT),
	DEFFMT(R32G32B32_TYPELESS),
	DEFFMT(R32G32B32_FLOAT),
	DEFFMT(R32G32B32_UINT),
	DEFFMT(R32G32B32_SINT),
	DEFFMT(R16G16B16A16_TYPELESS),
	DEFFMT(R16G16B16A16_FLOAT),
	DEFFMT(R16G16B16A16_UNORM),
	DEFFMT(R16G16B16A16_UINT),
	DEFFMT(R16G16B16A16_SNORM),
	DEFFMT(R16G16B16A16_SINT),
	DEFFMT(R32G32_TYPELESS),
	DEFFMT(R32G32_FLOAT),
	DEFFMT(R32G32_UINT),
	DEFFMT(R32G32_SINT),
	DEFFMT(R32G8X24_TYPELESS),
	DEFFMT(D32_FLOAT_S8X24_UINT),
	DEFFMT(R32_FLOAT_X8X24_TYPELESS),
	DEFFMT(X32_TYPELESS_G8X24_UINT),
	DEFFMT(R10G10B10A2_TYPELESS),
	DEFFMT(R10G10B10A2_UNORM),
	DEFFMT(R10G10B10A2_UINT),
	DEFFMT(R11G11B10_FLOAT),
	DEFFMT(R8G8B8A8_TYPELESS),
	DEFFMT(R8G8B8A8_UNORM),
	DEFFMT(R8G8B8A8_UNORM_SRGB),
	DEFFMT(R8G8B8A8_UINT),
	DEFFMT(R8G8B8A8_SNORM),
	DEFFMT(R8G8B8A8_SINT),
	DEFFMT(R16G16_TYPELESS),
	DEFFMT(R16G16_FLOAT),
	DEFFMT(R16G16_UNORM),
	DEFFMT(R16G16_UINT),
	DEFFMT(R16G16_SNORM),
	DEFFMT(R16G16_SINT),
	DEFFMT(R32_TYPELESS),
	DEFFMT(D32_FLOAT),
	DEFFMT(R32_FLOAT),
	DEFFMT(R32_UINT),
	DEFFMT(R32_SINT),
	DEFFMT(R24G8_TYPELESS),
	DEFFMT(D24_UNORM_S8_UINT),
	DEFFMT(R24_UNORM_X8_TYPELESS),
	DEFFMT(X24_TYPELESS_G8_UINT),
	DEFFMT(R8G8_TYPELESS),
	DEFFMT(R8G8_UNORM),
	DEFFMT(R8G8_UINT),
	DEFFMT(R8G8_SNORM),
	DEFFMT(R8G8_SINT),
	DEFFMT(R16_TYPELESS),
	DEFFMT(R16_FLOAT),
	DEFFMT(D16_UNORM),
	DEFFMT(R16_UNORM),
	DEFFMT(R16_UINT),
	DEFFMT(R16_SNORM),
	DEFFMT(R16_SINT),
	DEFFMT(R8_TYPELESS),
	DEFFMT(R8_UNORM),
	DEFFMT(R8_UINT),
	DEFFMT(R8_SNORM),
	DEFFMT(R8_SINT),
	DEFFMT(A8_UNORM),
	DEFFMT(R1_UNORM),
	DEFFMT(R9G9B9E5_SHAREDEXP),
	DEFFMT(R8G8_B8G8_UNORM),
	DEFFMT(G8R8_G8B8_UNORM),
	DEFFMT(BC1_TYPELESS),
	DEFFMT(BC1_UNORM),
	DEFFMT(BC1_UNORM_SRGB),
	DEFFMT(BC2_TYPELESS),
	DEFFMT(BC2_UNORM),
	DEFFMT(BC2_UNORM_SRGB),
	DEFFMT(BC3_TYPELESS),
	DEFFMT(BC3_UNORM),
	DEFFMT(BC3_UNORM_SRGB),
	DEFFMT(BC4_TYPELESS),
	DEFFMT(BC4_UNORM),
	DEFFMT(BC4_SNORM),
	DEFFMT(BC5_TYPELESS),
	DEFFMT(BC5_UNORM),
	DEFFMT(BC5_SNORM),
	DEFFMT(B5G6R5_UNORM),
	DEFFMT(B5G5R5A1_UNORM),
	DEFFMT(B8G8R8A8_UNORM),
	DEFFMT(B8G8R8X8_UNORM),
    { NULL, DXGI_FORMAT_UNKNOWN }
};

SValue g_pD3DX10FileTypes[] =
{
    { L"BMP",            D3DX10_IFF_BMP },
    { L"JPG",            D3DX10_IFF_JPG },
    { L"PNG",            D3DX10_IFF_PNG },
    { L"DDS",            D3DX10_IFF_DDS },
    { L"TIFF",           D3DX10_IFF_TIFF },
    { L"GIF",            D3DX10_IFF_GIF },
    { NULL,              D3DX10_IFF_DDS }
};

const LPWSTR FindName(DWORD value, SValue* list )
{
    for(SValue *ptr = list; ptr->pName != 0; ptr++)
    {
        if (ptr->dwValue == value)
            return ptr->pName;
    }

    return L"*UNKNOWN*";
}

WCHAR* ConvertTypeToString( WCHAR* strType )
{
    if( wcscmp(strType, L"1") == 0)
        return L"Provider";
    else 
        return L"Game";
}

//-----------------------------------------------------------------------------
void OutputGDFData( GDFData* pGDFData, D3DX10_IMAGE_INFO* pImage )
{
    wprintf( L"Language: %s (0x%0.4x)\n", pGDFData->strLanguage, pGDFData->wLanguage );
    wprintf( L"\tName: %s\n", pGDFData->strName );
    wprintf( L"\tDescription: %s\n", pGDFData->strDescription );
    wprintf( L"\tRelease Date: %s\n", pGDFData->strReleaseDate );
    wprintf( L"\tGenre: %s\n", pGDFData->strGenre );

    for( int iRating=0; iRating<16; iRating++ )
    {
        if( pGDFData->ratingData[iRating].strRatingSystem[0] == 0 )
            break;
        wprintf( L"\tRating: %s, %s", pGDFData->ratingData[iRating].strRatingSystem, pGDFData->ratingData[iRating].strRatingID );

        for( int iDescriptor=0; iDescriptor<MAX_DESCRIPTOR; iDescriptor++ )
        {
            if( pGDFData->ratingData[iRating].strDescriptor[iDescriptor][0] == 0 )
                break;
            wprintf( L", %s", pGDFData->ratingData[iRating].strDescriptor[iDescriptor] );
        }

        wprintf( L"\n" );
    }

    wprintf( L"\tVersion: %s\n", pGDFData->strVersion );
    wprintf( L"\tSaved Game Folder: %s\n", pGDFData->strSavedGameFolder );
    wprintf( L"\tWinSPR Min: %.1f\n", pGDFData->fSPRMin );
    wprintf( L"\tWinSPR Recommended: %.1f\n", pGDFData->fSPRRecommended );
    wprintf( L"\tDeveloper: %s\n", pGDFData->strDeveloper );
    wprintf( L"\tDeveloper Link: %s\n", pGDFData->strDeveloperLink );
    wprintf( L"\tPublisher: %s\n", pGDFData->strPublisher );
    wprintf( L"\tPublisher Link: %s\n", pGDFData->strPublisherLink );
    wprintf( L"\tType: %s\n", ConvertTypeToString(pGDFData->strType) );
    if( pGDFData->strType[0] == L'1' )
        wprintf( L"\tRSS: %s\n", pGDFData->strRSS );

    for( int iGame=0; iGame<MAX_GAMES; iGame++ )
    {   
        if( pGDFData->strExe[iGame][0] == 0 ) 
            break;
        wprintf( L"\tEXE: %s\n", pGDFData->strExe[iGame] );
    }

    if ( pImage && pImage->ImageFileFormat != D3DX10_IFF_FORCE_DWORD)
    {
        wprintf( L"\tThumbnail image: %dx%d (%s) %s\n", pImage->Width, pImage->Height,
                                                           FindName(pImage->Format, g_pD3DX10Formats),
                                                           FindName(pImage->ImageFileFormat, g_pD3DX10FileTypes) );
    }
}

//-----------------------------------------------------------------------------
void OutputWarning( LPCWSTR strMsg, ... )
{
    WCHAR strBuffer[1024];   
    va_list args;
    va_start(args, strMsg);
    StringCchVPrintfW( strBuffer, 512, strMsg, args );
    strBuffer[1023] = L'\0';
    va_end(args);
    wprintf( strBuffer );

    g_nNumberOfWarnings++;
}

//-----------------------------------------------------------------------------
bool FindRatingSystem( WCHAR* strRatingSystemGUID, GDFData* pGDFData, int* pRatingIndex )
{
    for( int iRating=0; iRating<16; iRating++ )
    {
        if( pGDFData->ratingData[iRating].strRatingSystemGUID[0] == 0 )
            return false;

        if( _wcsnicmp( strRatingSystemGUID, pGDFData->ratingData[iRating].strRatingSystemGUID, 256 ) == 0 )
        {
            *pRatingIndex = iRating;
            return true;
        }
    }

    return false;
}


//-----------------------------------------------------------------------------
bool g_bOuputLangHeader = false;
void EnsureOutputRatingHeader( GDFData* pGDFData1, GDFData* pGDFData2 )
{
    if( !g_bOuputLangHeader ) 
    {
        wprintf( L"\tComparing %s [0x%0.4x] with %s [0x%0.4x]\n", pGDFData1->strLanguage, pGDFData1->wLanguage, pGDFData2->strLanguage, pGDFData2->wLanguage );
        g_bOuputLangHeader = true;
    }
}

//-----------------------------------------------------------------------------
void OutputRatingWarning( GDFData* pGDFData1, GDFData* pGDFData2, LPCWSTR strMsg, ... )
{
    WCHAR strBuffer[1024];
    EnsureOutputRatingHeader( pGDFData1, pGDFData2 );
    
    va_list args;
    va_start(args, strMsg);
    StringCchVPrintfW( strBuffer, 512, strMsg, args );
    strBuffer[1023] = L'\0';
    va_end(args);

    OutputWarning( strBuffer );
}


//-----------------------------------------------------------------------------
bool CompareRatingSystems( GDFData* pGDFData1, GDFData* pGDFData2 )
{
    bool bWarningsFound = false;

    for( int iRating1=0; iRating1<16; iRating1++ )
    {
        GDFRatingData* pRating1 = &pGDFData1->ratingData[iRating1];
        if( pRating1->strRatingSystemGUID[0] == 0 )
            break;

        int iRating2 = 0;
        if( FindRatingSystem( pRating1->strRatingSystemGUID, pGDFData2, &iRating2 ) )
        {
            //wprintf( L"\t\tInfo: Rating system %s found in %s lang\n", pRating1->strRatingSystem, pGDFData2->strLanguage );

            GDFRatingData* pRating2 = &pGDFData2->ratingData[iRating2];
            if( _wcsnicmp( pRating1->strRatingID, pRating2->strRatingID, 256 ) != 0 )
            {
                OutputRatingWarning( pGDFData1, pGDFData2, L"\tWarning: %s rating mismatch: %s vs %s \n", pRating1->strRatingSystem, pRating1->strRatingID, pRating2->strRatingID );
                bWarningsFound = true;
            }                    
            else
            {
                //wprintf( L"\t\tInfo: %s rating match: %s vs %s \n", pRating1->strRatingSystem, pRating1->strRatingID, pRating2->strRatingID );
            }

            for( int iDescriptor1=0; iDescriptor1<MAX_DESCRIPTOR; iDescriptor1++ )
            {
                if( pRating1->strDescriptor[iDescriptor1][0] == 0 )
                    break;

                bool bFound = false;
                for( int iDescriptor2=0; iDescriptor2<MAX_DESCRIPTOR; iDescriptor2++ )
                {
                    if( pRating2->strDescriptor[iDescriptor2][0] == 0 )
                        break;

                    if( _wcsnicmp( pRating1->strDescriptor[iDescriptor1], pRating2->strDescriptor[iDescriptor2], 256 ) == 0 )
                    {
                        bFound = true;
                        break;
                    }
                }
                if( !bFound )
                {
                    OutputRatingWarning( pGDFData1, pGDFData2, L"\tWarning: %s rating descriptor not found: %s\n", pRating1->strRatingSystem, pRating1->strDescriptor[iDescriptor1] );
                    bWarningsFound = true;
                }
                else
                {
                    //wprintf( L"\t\tInfo: %s rating descriptor found: %s\n", pRating1->strRatingSystem, pRating1->strDescriptor[iDescriptor1] );
                }
            }
        }
        else
        {
            OutputRatingWarning( pGDFData1, pGDFData2, L"\tWarning: Rating system %s not found in %s lang\n", pRating1->strRatingSystem, pGDFData2->strLanguage );
            bWarningsFound = true;
        }
    }

    return bWarningsFound;
}


//-----------------------------------------------------------------------------
void CompareGDFData( GDFData* pGDFData1, GDFData* pGDFData2, bool bQuiet )
{
    g_bOuputLangHeader = false;
    bool bSPRWarningsFound = false;
    bool bGameIDWarningsFound = false;

    if( pGDFData1->fSPRMin != pGDFData2->fSPRMin )
    {
        bSPRWarningsFound = true;
        OutputRatingWarning( pGDFData1, pGDFData2, L"\t\tWarning: Mismatched SPR min: %.1f vs %.1f\n", pGDFData1->fSPRMin, pGDFData2->fSPRMin );
    }
    if( pGDFData1->fSPRRecommended != pGDFData2->fSPRRecommended )
    {
        bSPRWarningsFound = true;
        OutputRatingWarning( pGDFData1, pGDFData2, L"\t\tWarning: Mismatched SPR recommended: %.1f vs %.1f\n", pGDFData1->fSPRRecommended, pGDFData2->fSPRRecommended );
    }

    if( _wcsnicmp( pGDFData1->strGameID, pGDFData2->strGameID, 256 ) != 0 )
    {
        bGameIDWarningsFound = true;
        OutputRatingWarning( pGDFData1, pGDFData2, L"\t\tWarning: Mismatched game ID guid: %s vs %s\n", pGDFData1->strGameID, pGDFData2->strGameID );
    }

    bool bExeWarningsFound = false;
    for( int iGame=0; iGame<MAX_GAMES; iGame++ )
    {   
        if( pGDFData1->strExe[iGame][0] == 0 && pGDFData2->strExe[iGame][0] == 0 )
            break;
        if( _wcsnicmp( pGDFData1->strExe[iGame], pGDFData2->strExe[iGame], 256 ) != 0 )
        {
            bExeWarningsFound = true;
            OutputRatingWarning( pGDFData1, pGDFData2, L"\t\tWarning: Game EXE mismatch: %s vs %s\n", pGDFData1->strExe[iGame], pGDFData2->strExe[iGame] );
        }
    }

    bool bProviderWarningsFound = false;
    if( _wcsnicmp( pGDFData1->strType, pGDFData2->strType, 256 ) != 0 )
    {
        bProviderWarningsFound = true;
        OutputRatingWarning( pGDFData1, pGDFData2, L"\t\tWarning: Type (Game/Provider) mismatch between languages.\n" );
    }

    bool bWarningsFound1 = CompareRatingSystems( pGDFData1, pGDFData2 );
    bool bWarningsFound2 = CompareRatingSystems( pGDFData2, pGDFData1  );

    if( !bWarningsFound1 && !bWarningsFound2 && !bExeWarningsFound && !bSPRWarningsFound && !bGameIDWarningsFound && !bProviderWarningsFound )
    {
        // Matching Game ID, ratings, exes, and SPR data
        if( !bQuiet )
        {
            EnsureOutputRatingHeader( pGDFData1, pGDFData2 );
            wprintf( L"\t\tNo data mismatch found\n" );    
        }
    }
}

//-----------------------------------------------------------------------------
HRESULT ScanForWarnings( GDFData* pGDFDataList, D3DX10_IMAGE_INFO* pImageList, BOOL** ppIconEightBits, BOOL** ppIconThirtyTwoBits, int nNumLangs, bool bQuiet, bool bWarnMissingNEU )
{
    wprintf( L"Warnings:\n" );

    if ( bWarnMissingNEU )
    {
       // Loop through all languages and warn if there's no language neutral 
       bool bFoundNeutral = false;
       bool bFoundSublangNeutral = false;
       bool bFoundNonNeutral = false;
       for( int iLang1=0; iLang1<nNumLangs; iLang1++ )
       {
           GDFData* pGDFData1 = &pGDFDataList[iLang1];
           if( LOBYTE(pGDFData1->wLanguage) == LANG_NEUTRAL ) 
           {
               bFoundNeutral = true;

               if ( HIBYTE(pGDFData1->wLanguage) == SUBLANG_NEUTRAL )
                   bFoundSublangNeutral = true;

               if (pImageList)
               {
                   if (pImageList[iLang1].ImageFileFormat == D3DX10_IFF_FORCE_DWORD)
                       OutputWarning( L"\tWarning: Language neutral version of thumbnail is missing. Adding one is highly recommended to cover all other languages\n" );
               }
           }
           else
               bFoundNonNeutral = true;
       }
       if( !bFoundNeutral ) 
           OutputWarning( L"\tWarning: Language neutral not found.  Adding one is highly recommended to cover all other languages\n" );
       else
       {
           if ( bFoundSublangNeutral )
               OutputWarning( L"\tWarning: Language neutral resource is marked SUBLANG_NEUTRAL, recommend using SUBLANG_DEFAULT instead\n" );

           if ( !bFoundNonNeutral )
               OutputWarning( L"\tWarning: Found only language neutral resource, recommend using at least one non-neutral language in addition\n" );
       }
    }

    // Warn if there's any missing data or if there were XML validation warnings
    for( int iLang=0; iLang<nNumLangs; iLang++ )
    {
        WCHAR strHeader[256];
        StringCchPrintf( strHeader, 256, L"\t%s (0x%0.4x): ", pGDFDataList[iLang].strLanguage, pGDFDataList[iLang].wLanguage );

        if( !pGDFDataList[iLang].fV2GDF )
                    wprintf( L"%sThis GDF is using the v1 schema. Use of GDF v2 is recommended.\n", strHeader );

        if( pGDFDataList[iLang].strValidation[0] != 0  )
            OutputWarning( L"%s%s\n", strHeader, pGDFDataList[iLang].strValidation );
        else if( !bQuiet )
            wprintf( L"%sNo validation warnings found\n", strHeader );

        if( pGDFDataList[iLang].strPublisher[0] == 0 )
            OutputWarning( L"%sPublisher field is blank\n", strHeader );
        if( pGDFDataList[iLang].strPublisherLink[0] == 0 )
            OutputWarning( L"%sPublisher link field is blank\n", strHeader );
        if( pGDFDataList[iLang].strDeveloper[0] == 0 )
            OutputWarning( L"%sDeveloper field is blank\n", strHeader );
        if( pGDFDataList[iLang].strDeveloperLink[0] == 0 )
            OutputWarning( L"%sDeveloper link field is blank\n", strHeader );
        if( pGDFDataList[iLang].strGenre[0] == 0 )
            OutputWarning( L"%sGenre field is blank\n", strHeader );
        if( pGDFDataList[iLang].strDescription[0] == 0 )
            OutputWarning( L"%sDescription field is blank\n", strHeader );
        if( pGDFDataList[iLang].fSPRMin == pGDFDataList[iLang].fSPRRecommended )
            OutputWarning( L"%sWinSPR minimum and recommended are the same.  Ensure this is intended.\n", strHeader );
        if( pGDFDataList[iLang].fSPRMin > pGDFDataList[iLang].fSPRRecommended )
            OutputWarning( L"%sWinSPR minimum should be less than or equal to recommended.\n", strHeader );
        if( pGDFDataList[iLang].strExe[0][0] == 0 )
            OutputWarning( L"%sNo EXEs listed\n", strHeader );
        if( pGDFDataList[iLang].ratingData[0].strRatingSystemGUID[0] == 0 )
            OutputWarning( L"%sNo ratings data found\n", strHeader );
        if( pGDFDataList[iLang].strType[0] == L'1' && pGDFDataList[iLang].strRSS[0] == 0 )
            OutputWarning( L"%sRSS link field is blank\n", strHeader );

        bool bPEGIFound = false;
        bool bPEGIFinlandFound = false;
        
        for( int iRating=0; iRating<16; iRating++ )
        {
            if( pGDFDataList[iLang].ratingData[iRating].strRatingSystemGUID[0] == 0 )
                break;

            if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strRatingSystemGUID, L"{36798944-B235-48AC-BF21-E25671F597EE}", 256 ) == 0 )
            {
                bPEGIFound = true;
            }

            if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strRatingSystemGUID, L"{7F2A4D3A-23A8-4123-90E7-D986BF1D9718}", 256 ) == 0 )
            {
                bPEGIFinlandFound = true;
            }

            if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strRatingIDGUID, L"{6B9EB3C0-B49A-4708-A6E6-F5476CE7567B}", 256 ) == 0 )
            {
                OutputWarning( L"%sUnsupported CERO rating found.  Use latest GDFMaker to fix.\n", strHeader );
            }
            
            if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strRatingSystemGUID, L"{768BD93D-63BE-46A9-8994-0B53C4B5248F}", 256 ) == 0 )
            {
                for( int iDescriptor=0; iDescriptor<MAX_DESCRIPTOR; iDescriptor++ )
                {
                    if( pGDFDataList[iLang].ratingData[iRating].strDescriptor[iDescriptor][0] == 0 )
                        break;
                    if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strDescriptorGUID[iDescriptor], L"{5990705B-1E85-4435-AE11-129B9319FF09}", 256 ) == 0 )
                    {                               
                        OutputWarning( L"%sDeprecated ESRB 'Gambling' descriptor found.  Use 'Simulated Gambling' instead.\n", strHeader );
                    }
                    if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strDescriptorGUID[iDescriptor], L"{E9476FB8-0B11-4209-9A7D-CBA553C1555D}", 256 ) == 0 )
                    {                               
                        OutputWarning( L"%sDeprecated ESRB 'Mature Sexual Themes' descriptor found.  Use 'Sexual Themes' instead.\n", strHeader );
                    }
                    if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strDescriptorGUID[iDescriptor], L"{1A796A5D-1E25-4862-9443-1550578FF4C4}", 256 ) == 0 )
                    {                               
                        OutputWarning( L"%sDeprecated ESRB 'Mild Language' descriptor found.  Use 'Language' instead.\n", strHeader );
                    }
                    if( _wcsnicmp( pGDFDataList[iLang].ratingData[iRating].strDescriptorGUID[iDescriptor], L"{40B262D1-11AA-43C2-B7BA-63A9F5756A06}", 256 ) == 0 )
                    {                               
                        OutputWarning( L"%sDeprecated ESRB 'Mild Lyrics' descriptor found.  Use 'Lyrics' instead.\n", strHeader );
                    }
                }
            }
        }

        if ( bPEGIFinlandFound)
        {
            if ( bPEGIFound )
            {
                OutputWarning( L"%sThe PEGI-fi rating system has been deprecated and should be removed from the project.\n", strHeader);
            }
            else
            {
                OutputWarning( L"%sThe PEGI-fi rating system has been deprecated, PEGI is now the Finnish locale rating system.\n", strHeader);
            }
        }

        if ( pImageList )
        {
            if ( pImageList[iLang].ImageFileFormat != D3DX10_IFF_FORCE_DWORD)
            {
                OutputWarning( L"%sThumbnail image is not recommended, please use a 256x256 icon.\n", strHeader );

                if ( pImageList[iLang].ImageFileFormat == D3DX10_IFF_DDS )
                    OutputWarning( L"%sDDS Format is not supported for GE thumbnail image data, use PNG.\n", strHeader );
                else if ( pImageList[iLang].ImageFileFormat != D3DX10_IFF_PNG )
                    OutputWarning( L"%sPNG format is recommended for GE thumbnail image data.\n", strHeader );

                if ( pImageList[iLang].Width < 256 || pImageList[iLang].Height < 256 )
                    OutputWarning( L"%s256x256 is the recommended size of GE thumbnail image data (%d x %d).\n",
                                   strHeader, pImageList[iLang].Width, pImageList[iLang].Height );
            }
        }

        if (ppIconEightBits)
        {
            if (ppIconEightBits[iLang])
            {
                for (int res=0; res<4; res++)
                {
                    if (!ppIconEightBits[iLang][res])
                    {
                        OutputWarning( L"%s%dx%d 8bits icon is missing.\n", strHeader, iconResolution[res], iconResolution[res] );
                    }
                }
            }
        }

        if (ppIconThirtyTwoBits)
        {
            if (ppIconThirtyTwoBits[iLang])
            {
                for (int res=0; res<4; res++)
                {
                    if (!ppIconThirtyTwoBits[iLang][res])
                    {
                        OutputWarning( L"%s%dx%d 32bits icon is missing.\n", strHeader, iconResolution[res], iconResolution[res] );
                    }
                }
            }
        }

        wprintf( L"\n" );
        
    }

    // Loop through all languages comparing GDF data and printing warnings
    for( int iLang1=0; iLang1<nNumLangs; iLang1++ )
    {
        for( int iLang2=iLang1+1; iLang2<nNumLangs; iLang2++ )
        {
            GDFData* pGDFData1 = &pGDFDataList[iLang1];
            GDFData* pGDFData2 = &pGDFDataList[iLang2];

            CompareGDFData( pGDFData1, pGDFData2, bQuiet );
        }
    }

    if( g_nNumberOfWarnings == 0 )
    {
        wprintf( L"\tNo warnings found\n" );
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT ProcessGDF( WCHAR* strGDFPath, bool bMuteWarnings, bool bMuteGDF, bool bQuiet )
{
    HRESULT hr;

    CRatingsDB ratingsDB;
    ratingsDB.LoadDB();

    CGDFParse gdfParse;

    GDFData* pGDFData = new GDFData;
    ZeroMemory( pGDFData, sizeof(GDFData) );

    hr = GetGDFDataFromGDF( pGDFData, strGDFPath );
    if( FAILED(hr) )
    {
        wprintf( L"Couldn't load GDF XML data from: %s\n", strGDFPath );
        if( pGDFData->strValidation[0] != 0 )
        {
            wprintf( L"%s\n", pGDFData->strValidation );
        }
        delete pGDFData;
        return E_FAIL;
    }

    if( !bMuteGDF )
    {
        OutputGDFData( pGDFData, NULL );
    }

    if( !bMuteWarnings )
        ScanForWarnings( pGDFData, NULL, NULL, NULL, 1, bQuiet, false );

    delete pGDFData;

    return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT ProcessBIN( WCHAR* strGDFBinPath, bool bMuteWarnings, bool bMuteGDF, bool bQuiet )
{
    HRESULT hr;

    CRatingsDB ratingsDB;
    ratingsDB.LoadDB();

    CGDFParse gdfParse;
    hr = gdfParse.EnumLangs( strGDFBinPath );

    if ( FAILED(hr) )
    {
        LPVOID buff = 0;
        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                       NULL, (hr & 0xffff), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &buff, 0, NULL );
        wprintf( L"Failed processing binary: %s\n(0x%08x) %s\n", strGDFBinPath, hr, (LPWSTR) buff );
        LocalFree( buff );
        return E_FAIL;
    }

    if ( gdfParse.GetNumLangs() == 0 )
    {
        wprintf( L"Could not locate any GDF language resources in binary: %s\n", strGDFBinPath );
        return E_FAIL;
    }

    const int nLangs = gdfParse.GetNumLangs();

    // Load GDF XML data
    GDFData* pGDFDataList = new GDFData[nLangs];
    ZeroMemory( pGDFDataList, sizeof(GDFData)*nLangs );

    for( int iLang=0; iLang<nLangs; iLang++ )
    {
        WORD wLang = gdfParse.GetLang( iLang );

        hr = GetGDFDataFromBIN( &pGDFDataList[iLang], strGDFBinPath, wLang );
        if( FAILED(hr) )
        {
            wprintf( L"Couldn't load GDF XML data from: %s (wLang:0x%0.4x)\n", strGDFBinPath, wLang );
            if( pGDFDataList[iLang].strValidation[0] != 0 )
            {
                wprintf( L"%s\n", pGDFDataList[iLang].strValidation );
            }
            continue;
        }
    }

    // Load GDF Thumbnail data
    D3DX10_IMAGE_INFO* pImageList = new D3DX10_IMAGE_INFO[ nLangs ];
    ZeroMemory( pImageList, sizeof(D3DX10_IMAGE_INFO)*nLangs );

    for( int iLang=0; iLang<nLangs; iLang++ )
    {
        WORD wLang = gdfParse.GetLang( iLang );

        if ( FAILED(gdfParse.ExtractGDFThumbnail( strGDFBinPath, &pImageList[iLang], wLang )) )
        {
            pImageList[iLang].ImageFileFormat = D3DX10_IFF_FORCE_DWORD;
        }
        
    }

    BOOL** ppIconEightBits = new BOOL*[nLangs];
    for( int iLang=0; iLang<nLangs; iLang++ )
    {
        ppIconEightBits[iLang] = new BOOL[4];
        ZeroMemory( ppIconEightBits[iLang], sizeof(BOOL)*4 );
    }
    
    BOOL** ppIconThirtyTwoBits = new BOOL*[nLangs];
    for( int iLang=0; iLang<nLangs; iLang++ )
    {
        ppIconThirtyTwoBits[iLang] = new BOOL[4];
        ZeroMemory( ppIconThirtyTwoBits[iLang], sizeof(BOOL)*4 );
    }

    if( !bMuteGDF )
    {
        for( int iLang=0; iLang<nLangs; iLang++ )
        {
            OutputGDFData( &pGDFDataList[iLang], &pImageList[iLang] );
            
            WORD wLang = gdfParse.GetLang( iLang );
            gdfParse.OutputGDFIconInfo( strGDFBinPath, ppIconEightBits[iLang], ppIconThirtyTwoBits[iLang], wLang );
        }
    }

    if( !bMuteWarnings )
    {
        ScanForWarnings( pGDFDataList, pImageList, ppIconEightBits, ppIconThirtyTwoBits, nLangs, bQuiet, true );

        if ( !gdfParse.IsIconPresent( strGDFBinPath ) )
            OutputWarning( L"\tWarning: Icon not found. Adding one is highly recommended!\n" );
    }
    
    delete [] pGDFDataList;
    delete [] pImageList;

    for( int iLang=0; iLang<nLangs; iLang++ )
    {
        delete [] ppIconEightBits[iLang];
        delete [] ppIconThirtyTwoBits[iLang];
    }
    
    delete [] ppIconEightBits;
    delete [] ppIconThirtyTwoBits;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Entry point to the program. Initializes everything, and pops
// up a message box with the results of the GameuxInstallHelper calls
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    SETTINGS settings;
    memset(&settings, 0, sizeof(SETTINGS));
    settings.bQuiet = true;

    if( !ParseCommandLine( &settings ) )
        return 0;

    HRESULT hr;

    if (settings.bGDFInsteadOfBinary)
    {
        hr = ProcessGDF( settings.strGDFBinPath, settings.bMuteWarnings, settings.bMuteGDF, settings.bQuiet);
    }
    else
    {
        hr = ProcessBIN( settings.strGDFBinPath, settings.bMuteWarnings, settings.bMuteGDF, settings.bQuiet );
    }

    if( SUCCEEDED(hr) && g_nNumberOfWarnings == 0 )
        return 0;
    else
        return 1;
}


//--------------------------------------------------------------------------------------
// Parses the command line for parameters.  See DXUTInit() for list 
//--------------------------------------------------------------------------------------
bool ParseCommandLine( SETTINGS* pSettings )
{
    WCHAR* strCmdLine;
//    WCHAR* strArg;

    int nNumArgs;
    WCHAR** pstrArgList = CommandLineToArgvW( GetCommandLine(), &nNumArgs );
    for( int iArg=1; iArg<nNumArgs; iArg++ )
    {
        strCmdLine = pstrArgList[iArg];

        // Handle flag args
        if( *strCmdLine == L'/' || *strCmdLine == L'-' )
        {
            strCmdLine++;

            if( IsNextArg( strCmdLine, L"gdf" ) )
            {
                pSettings->bGDFInsteadOfBinary = true;
                continue;
            }

            if( IsNextArg( strCmdLine, L"mutegdf" ) )
            {
                pSettings->bMuteGDF = true;
                continue;
            }

            if( IsNextArg( strCmdLine, L"mutewarnings" ) )
            {
                pSettings->bMuteWarnings = true;
                continue;
            }

            if( IsNextArg( strCmdLine, L"noisy" ) )
            {
                pSettings->bQuiet = false;
                continue;
            }

            if( IsNextArg( strCmdLine, L"?" ) )
            {
                DisplayUsage();
                return false;
            }
        }
        else 
        {
            // Handle non-flag args as separate input files
            StringCchPrintf( pSettings->strGDFBinPath, MAX_PATH, L"%s", strCmdLine );
            continue;
        }
    }

    if( pSettings->strGDFBinPath[0] == 0 )
    {
        DisplayUsage();
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg )
{
    int nArgLen = (int) wcslen(strArg);
    if( _wcsnicmp( strCmdLine, strArg, nArgLen ) == 0 && strCmdLine[nArgLen] == 0 )
        return true;

    return false;
}


//--------------------------------------------------------------------------------------
void DisplayUsage()
{
    wprintf( L"\n" );
    wprintf( L"GDFTrace - a command line tool that displays GDF metadata contained\n" );
    wprintf( L"           in a binary and highlights any warnings\n" );
    wprintf( L"\n" );
    wprintf( L"Usage: GDFTrace.exe [options] <gdf binary>\n" );
    wprintf( L"\n" );
    wprintf( L"where:\n" ); 
    wprintf( L"\n" ); 
    wprintf( L"  [/mutegdf]     \tmutes output of GDF data\n" );
    wprintf( L"  [/mutewarnings]\tmutes output of warnings\n" );
    wprintf( L"  [/noisy]       \tenables output of success\n" );
    wprintf( L"  [/gdf]         \ttest .gdf file directly instead of embedded binary\n" );
    wprintf( L"  <gdf binary>\tThe path to the GDF binary\n" );
    wprintf( L"\n" );
    wprintf( L"After running, %%ERRORLEVEL%% will be 0 if no warnings are found,\n" );
    wprintf( L"and 1 otherwise.\n" );
    wprintf( L"\n" );
    wprintf( L"As an example, you can use GDFExampleBinary.dll found in the DXSDK.\n" );
}
