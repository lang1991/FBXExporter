//--------------------------------------------------------------------------------------
// File: RatingsDB.cpp
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
#define ID_RATINGS_XML 2

#include <windows.h>
#include <rpcsal.h>
#include <gameux.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <assert.h>
#include <shellapi.h>
#include <shlobj.h>
#include "RatingsDB.h"


//--------------------------------------------------------------------------------------
CRatingsDB::CRatingsDB(void)
{
    HRESULT hr = CoInitialize( 0 );
    m_bCleanupCOM = SUCCEEDED(hr); 
    m_pRootNode = NULL;
}


//--------------------------------------------------------------------------------------
CRatingsDB::~CRatingsDB(void)
{
    SAFE_RELEASE( m_pRootNode );
    if( m_bCleanupCOM )
        CoUninitialize();
}


//--------------------------------------------------------------------------------------
HRESULT CRatingsDB::LoadDB()
{
    IXMLDOMDocument *pDoc = NULL;

    // Find resource will pick the right ID_GDF_XML_STR based on the current language
    HRSRC hrsrc = FindResource( NULL, MAKEINTRESOURCE(ID_RATINGS_XML), L"DATA" ); 
    if( hrsrc ) 
    { 
        HGLOBAL hgResource = LoadResource( NULL, hrsrc ); 
        if( hgResource ) 
        { 
            BYTE* pResourceBuffer = (BYTE*)LockResource( hgResource ); 
            if( pResourceBuffer ) 
            { 
                DWORD dwGDFXMLSize = SizeofResource( NULL, hrsrc );
                if( dwGDFXMLSize )
                {
                    // HGLOBAL from LoadResource() needs to be copied for CreateStreamOnHGlobal() to work
                    HGLOBAL hgResourceCopy = GlobalAlloc( GMEM_MOVEABLE, dwGDFXMLSize );
                    if( hgResource )
                    {
                        LPVOID pCopy = GlobalLock( hgResourceCopy );
                        if( pCopy )
                        {
                            CopyMemory( pCopy, pResourceBuffer, dwGDFXMLSize );
                            GlobalUnlock( hgResource );

                            IStream* piStream = NULL;
                            CreateStreamOnHGlobal( hgResourceCopy, TRUE, &piStream ); 
                            if( piStream )
                            {
                                IXMLDOMDocument *pDoc = NULL;
                                HRESULT hr;

                                // Load the XML into a IXMLDOMDocument object
                                hr = CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                                       IID_IXMLDOMDocument, (void**)&pDoc );
                                if( SUCCEEDED(hr) ) 
                                {
                                    IPersistStreamInit* pPersistStreamInit = NULL;
                                    hr = pDoc->QueryInterface( IID_IPersistStreamInit, (void**) &pPersistStreamInit );
                                    if( SUCCEEDED(hr) ) 
                                    {
                                        hr = pPersistStreamInit->Load( piStream );
                                        if( SUCCEEDED(hr) ) 
                                        {
                                            // Get the root node to the XML doc and store it 
                                            pDoc->QueryInterface( IID_IXMLDOMNode, (void**)&m_pRootNode );
                                        }
                                        SAFE_RELEASE( pPersistStreamInit );
                                    }
                                    SAFE_RELEASE( pDoc );
                                }
                                SAFE_RELEASE( piStream );
                            }
                        }
                        GlobalFree( hgResourceCopy );
                    }
                }
            } 
        } 
    } 

    if( m_pRootNode )
        return S_OK;
    else
        return E_FAIL;
}


//--------------------------------------------------------------------------------------
HRESULT CRatingsDB::GetRatingSystemName( WCHAR* strRatingSystemGUID, WCHAR* strDest, int cchDest )
{
    HRESULT hr;
    IXMLDOMNode* pNode = NULL;
    WCHAR str[512];

    StringCchCopy( strDest, cchDest, strRatingSystemGUID );

    WCHAR strRatingSystemGUIDUpper[512];
    StringCchCopy( strRatingSystemGUIDUpper, 512, strRatingSystemGUID );
    _wcsupr( strRatingSystemGUIDUpper );

    StringCchPrintf( str, 512, L"//Ratings/RatingSystem[ @ID = \"%s\" ]", strRatingSystemGUIDUpper );    

    hr = m_pRootNode->selectSingleNode( str, &pNode );
    if( SUCCEEDED(hr) && pNode != NULL )
    {
        hr = GetAttribFromNode( pNode, L"Text", strDest, cchDest );
        SAFE_RELEASE( pNode );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CRatingsDB::GetRatingIDName( WCHAR* strRatingSystemGUID, WCHAR* strRatingIDGUID, WCHAR* strDest, int cchDest )
{
    StringCchCopy( strDest, cchDest, strRatingIDGUID );

    WCHAR strRatingSystemGUIDUpper[512];
    StringCchCopy( strRatingSystemGUIDUpper, 512, strRatingSystemGUID );
    _wcsupr( strRatingSystemGUIDUpper );

    WCHAR strRatingIDGUIDUpper[512];
    StringCchCopy( strRatingIDGUIDUpper, 512, strRatingIDGUID );
    _wcsupr( strRatingIDGUIDUpper );

    HRESULT hr;
    IXMLDOMNode* pRatingSystemNode = NULL;
    WCHAR str[512];
    StringCchPrintf( str, 512, L"//Ratings/RatingSystem[@ID='%s']", strRatingSystemGUIDUpper );

    hr = m_pRootNode->selectSingleNode( str, &pRatingSystemNode );
    if( SUCCEEDED(hr) && pRatingSystemNode != NULL )
    {
        IXMLDOMNode* pRatingIDNode = NULL;
        StringCchPrintf( str, 512, L"Rating[@ID='%s']", strRatingIDGUIDUpper );
        hr = pRatingSystemNode->selectSingleNode( str, &pRatingIDNode );
        if( SUCCEEDED(hr) && pRatingIDNode != NULL )
        {
            hr = GetAttribFromNode( pRatingIDNode, L"Text", strDest, cchDest );
            SAFE_RELEASE( pRatingIDNode );
        }
        SAFE_RELEASE( pRatingSystemNode );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CRatingsDB::GetDescriptorName( WCHAR* strRatingSystemGUID, WCHAR* strDescriptorGUID, WCHAR* strDest, int cchDest )
{
    StringCchCopy( strDest, cchDest, strDescriptorGUID );

    WCHAR strRatingSystemGUIDUpper[512];
    StringCchCopy( strRatingSystemGUIDUpper, 512, strRatingSystemGUID );
    _wcsupr( strRatingSystemGUIDUpper );

    WCHAR strDescriptorGUIDUpper[512];
    StringCchCopy( strDescriptorGUIDUpper, 512, strDescriptorGUID );
    _wcsupr( strDescriptorGUIDUpper );

    HRESULT hr;
    IXMLDOMNode* pRatingSystemNode = NULL;
    WCHAR str[512];
    StringCchPrintf( str, 512, L"//Ratings/RatingSystem[@ID='%s']", strRatingSystemGUIDUpper );

    hr = m_pRootNode->selectSingleNode( str, &pRatingSystemNode );
    if( SUCCEEDED(hr) && pRatingSystemNode != NULL )
    {
        IXMLDOMNode* pRatingIDNode = NULL;
        StringCchPrintf( str, 512, L"Descriptor[@ID='%s']", strDescriptorGUIDUpper );
        hr = pRatingSystemNode->selectSingleNode( str, &pRatingIDNode );
        if( SUCCEEDED(hr) && pRatingIDNode != NULL )
        {
            hr = GetAttribFromNode( pRatingIDNode, L"Text", strDest, cchDest );
            SAFE_RELEASE( pRatingIDNode );
        }
        SAFE_RELEASE( pRatingSystemNode );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CRatingsDB::GetAttribFromNode( IXMLDOMNode* pNode, WCHAR* strAttrib, WCHAR* strDest, int cchDest )
{
    IXMLDOMNamedNodeMap *pIXMLDOMNamedNodeMap = NULL;
    BSTR bstrAttributeName = ::SysAllocString( strAttrib );
    IXMLDOMNode* pIXMLDOMNode = NULL;    
    bool bFound = false;

    HRESULT hr;
    VARIANT v;
    hr = pNode->get_attributes( &pIXMLDOMNamedNodeMap );
    if(SUCCEEDED(hr) && pIXMLDOMNamedNodeMap)
    {
        hr = pIXMLDOMNamedNodeMap->getNamedItem( bstrAttributeName, &pIXMLDOMNode );
        if(SUCCEEDED(hr) && pIXMLDOMNode)
        {
            pIXMLDOMNode->get_nodeValue(&v);
            if( SUCCEEDED(hr) && v.vt == VT_BSTR )
            {
                StringCchCopy( strDest, cchDest, v.bstrVal );
                bFound = true;
            }
            VariantClear(&v);
            SAFE_RELEASE( pIXMLDOMNode );
        }
        SAFE_RELEASE( pIXMLDOMNamedNodeMap );
    }

    ::SysFreeString(bstrAttributeName);
    bstrAttributeName = NULL;

    if( !bFound )
        return E_FAIL;
    else
        return S_OK;
}

