//----------------------------------------------------------------------------
// File: DXErr.cpp
//
// Desc: The DXErr sample allows users to enter a numberical HRESULT and get back
//       the string match its define.  For example, entering 0x8878000a will
//       return DSERR_ALLOCATED.
//
// Copyright (c) Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <math.h>
#include <basetsd.h>
#include <dxerr.h>
#include <tchar.h>
#include "resource.h"

#include <strsafe.h>

//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
VOID OnInitDialog( HWND hDlg );
VOID LookupValue( HWND hDlg );
BOOL g_bUseHex = TRUE;

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------

// Latest facilities from winerror.h, not in VS 2005 version of winerror.h
#ifndef FACILITY_USERMODE_VOLMGR
#define FACILITY_USERMODE_VOLMGR         56
#endif

#ifndef FACILITY_USERMODE_VIRTUALIZATION
#define FACILITY_USERMODE_VIRTUALIZATION 55
#endif

#ifndef FACILITY_BCD
#define FACILITY_BCD                     57
#endif

// DirectX SDK Components
#define FACILITY_D3D             0x876
#define FACILITY_D3DX            0x877
#define FACILITY_DSOUND_DMUSIC	 0x878
#define FACILITY_D3D10           0x879
#define FACILITY_DXGI            0x87a
#define FACILITY_XAUDIO2         0x896
#define FACILITY_XAPO            0x897
#define FACILITY_XACTENGINE      0xac7
#define FACILITY_D2D             0x899
#define FACILITY_DWRITE          0x898

// D3D11 and the system audio component conflict so might be either
#define FACILITY_D3D11_OR_AE     0x87c

// Related system components
#define FACILITY_APO             0x87d
#define FACILITY_LEAP            0x888
#define FACILITY_WSAPI           0x889

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application.  Since we use a simple dialog for 
//       user interaction we don't need to pump messages.
//-----------------------------------------------------------------------------
INT APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, 
                      INT nCmdShow )
{
    // Display the main dialog box.
    DialogBox( hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc );

    return 0;
}




//-----------------------------------------------------------------------------
// Name: MainDlgProc()
// Desc: Handles dialog messages
//-----------------------------------------------------------------------------
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg ) 
    {
        case WM_INITDIALOG:
            OnInitDialog( hDlg );
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_LOOKUP:
                    LookupValue( hDlg );
                    break;

                case IDC_HEX:
                    g_bUseHex = TRUE;
                    break;

                case IDC_DECIMAL:
                    g_bUseHex = FALSE;
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;

                default:
                    return FALSE; // Didn't handle message
            }
            break;

        case WM_DESTROY:
            break; 

        default:
            return FALSE; // Didn't handle message
    }

    return TRUE; // Handled message
}




//-----------------------------------------------------------------------------
// Name: OnInitDialog()
// Desc: Initializes the dialogs (sets up UI controls, etc.)
//-----------------------------------------------------------------------------
VOID OnInitDialog( HWND hDlg )
{
    // Load the icon
#ifdef _WIN64
    HINSTANCE hInst = (HINSTANCE) GetWindowLongPtr( hDlg, GWLP_HINSTANCE );
#else
    HINSTANCE hInst = (HINSTANCE) GetWindowLong( hDlg, GWL_HINSTANCE );
#endif
    HICON hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDR_MAINFRAME ) );

    // Set the icon for this dialog.
    SendMessage( hDlg, WM_SETICON, ICON_BIG,   (LPARAM) hIcon );  // Set big icon
    SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );  // Set small icon

    CheckDlgButton( hDlg, IDC_HEX, BST_CHECKED );

    SendMessage( hDlg, EM_LIMITTEXT, 20, 0 );  
}



#define CHK_FAC(x)  case x: StringCchCopy( strFacility, 100, TEXT(#x) ); break;

//-----------------------------------------------------------------------------
// Name: LookupValue()
// Desc: 
//-----------------------------------------------------------------------------
VOID LookupValue( HWND hDlg )
{
    HRESULT hrErr = 0;
    TCHAR strValue[MAX_PATH];
    const TCHAR* strHRESULT;
    const TCHAR* strDescription;
    TCHAR strHRESULTCopy[MAX_PATH*2];
    int nIndex;
    int nPower = 0;
    int nPowerDec = 1;
    int nDigit = 0;
    GetDlgItemText( hDlg, IDC_VALUE, strValue, MAX_PATH );

    StringCchLength(strValue, MAX_PATH, (size_t *)&nIndex);
    nIndex--;

    // skip whitespace
    while( nIndex >= 0 )
    {
    	if( strValue[nIndex] != ' ' && 
            strValue[nIndex] != 'L' )
    	    break;
    	    
        nIndex--;
    }

    bool bFoundDigit = false;
    while( nIndex >= 0 )
    {
        // Convert to uppercase
        if( strValue[nIndex] >= 'a' && strValue[nIndex] <= 'z' )
            strValue[nIndex] += 'A' - 'a';

        if( g_bUseHex && strValue[nIndex] >= 'A' && strValue[nIndex] <= 'F' )
            nDigit = strValue[nIndex] - 'A' + 10;
        else if( strValue[nIndex] >= '0' && strValue[nIndex] <= '9' )
            nDigit = strValue[nIndex] - '0';
        else if( strValue[nIndex] == '-' )
        {
            hrErr = -hrErr;
            break;
        }
        else
        {
            // break if we've found a number, but don't break otherwise
            // This will skip any random letters in the front & end of the string
            if( bFoundDigit )
            {
                break;
            }
            else
            {
                nIndex--;
                continue;
            }
        }

        bFoundDigit = true;
        if( g_bUseHex )
            hrErr += ( nDigit << (nPower*4) );
        else
            hrErr += ( nDigit * nPowerDec );

        nPowerDec *= 10;
        nIndex--;
        nPower++;
    }

    if( hrErr == 0 && !bFoundDigit )
    {
        SetDlgItemText( hDlg, IDC_MESSAGE, TEXT("Invalid input") );
    }
    else
    {
        TCHAR strInternals[100];
        TCHAR strFacility[100];
        
        // Use dxerr.lib to lookup HRESULT.
        strHRESULT = DXGetErrorString( hrErr );
        StringCchPrintf( strHRESULTCopy, MAX_PATH*2, TEXT("HRESULT: 0x%0.8x (%lu)"), 
                    hrErr, hrErr );
        StringCchCat( strHRESULTCopy, MAX_PATH*2, TEXT("\r\nName: ") );
        StringCchCat( strHRESULTCopy, MAX_PATH*2, strHRESULT );

        strDescription = DXGetErrorDescription( hrErr );

        TCHAR* strTemp;
        while( ( strTemp = _tcschr( strHRESULTCopy, '&') ) != '\0' )
        {
            strTemp[0] = '\r';
            strTemp[1] = '\n';
        }

        if( lstrlen(strDescription) > 0 )
        {
            StringCchCat( strHRESULTCopy, MAX_PATH*2, TEXT("\r\nDescription: ") );
            StringCchCat( strHRESULTCopy, MAX_PATH*2, strDescription );
        }

        StringCchPrintf( strInternals, 100, TEXT("\r\nSeverity code: %s"), 
                    ( HRESULT_SEVERITY(hrErr) == 1 ) ? TEXT("Failed") : TEXT("Success") );
        StringCchCat( strHRESULTCopy, MAX_PATH*2, strInternals );

        int nFacility = HRESULT_FACILITY(hrErr);
        switch( nFacility )
        {
            // Windows winerr.h facilities
            CHK_FAC(FACILITY_WINRM)
            CHK_FAC(FACILITY_WINDOWSUPDATE)
            CHK_FAC(FACILITY_WINDOWS_DEFENDER)
            CHK_FAC(FACILITY_WINDOWS_CE)
            CHK_FAC(FACILITY_WINDOWS)
            CHK_FAC(FACILITY_USERMODE_VOLMGR)
            CHK_FAC(FACILITY_USERMODE_VIRTUALIZATION)
            CHK_FAC(FACILITY_URT)
            CHK_FAC(FACILITY_UMI)
            CHK_FAC(FACILITY_TPM_SOFTWARE)
            CHK_FAC(FACILITY_TPM_SERVICES)
            CHK_FAC(FACILITY_SXS)
            CHK_FAC(FACILITY_STORAGE)
            CHK_FAC(FACILITY_STATE_MANAGEMENT)
            CHK_FAC(FACILITY_SCARD)
            CHK_FAC(FACILITY_SHELL)
            CHK_FAC(FACILITY_SETUPAPI)
            CHK_FAC(FACILITY_SECURITY) // same as FACILITY_SSPI
            CHK_FAC(FACILITY_RPC)
            CHK_FAC(FACILITY_PLA)
            CHK_FAC(FACILITY_WIN32)
            CHK_FAC(FACILITY_CONTROL)
            CHK_FAC(FACILITY_NULL)
            CHK_FAC(FACILITY_NDIS)
            CHK_FAC(FACILITY_METADIRECTORY)
            CHK_FAC(FACILITY_MSMQ)
            CHK_FAC(FACILITY_MEDIASERVER)
            CHK_FAC(FACILITY_INTERNET)
            CHK_FAC(FACILITY_ITF)
            CHK_FAC(FACILITY_USERMODE_HYPERVISOR)
            CHK_FAC(FACILITY_HTTP)
            CHK_FAC(FACILITY_GRAPHICS)
            CHK_FAC(FACILITY_FWP)
            CHK_FAC(FACILITY_FVE)
            CHK_FAC(FACILITY_USERMODE_FILTER_MANAGER)
            CHK_FAC(FACILITY_DPLAY)
            CHK_FAC(FACILITY_DISPATCH)
            CHK_FAC(FACILITY_DIRECTORYSERVICE)
            CHK_FAC(FACILITY_CONFIGURATION)
            CHK_FAC(FACILITY_COMPLUS)
            CHK_FAC(FACILITY_USERMODE_COMMONLOG)
            CHK_FAC(FACILITY_CMI)
            CHK_FAC(FACILITY_CERT)
            CHK_FAC(FACILITY_BCD)
            CHK_FAC(FACILITY_BACKGROUNDCOPY)
            CHK_FAC(FACILITY_ACS)
            CHK_FAC(FACILITY_AAF)

            // DirectX SDK Components
            CHK_FAC(FACILITY_D3D)
            CHK_FAC(FACILITY_D3DX)
            CHK_FAC(FACILITY_DSOUND_DMUSIC)
            CHK_FAC(FACILITY_D3D10)
            CHK_FAC(FACILITY_DXGI)
            CHK_FAC(FACILITY_XAUDIO2)
            CHK_FAC(FACILITY_XAPO)
            CHK_FAC(FACILITY_XACTENGINE)
            CHK_FAC(FACILITY_D3D11_OR_AE)
            CHK_FAC(FACILITY_D2D)
            CHK_FAC(FACILITY_DWRITE)

            // Related system components
            CHK_FAC(FACILITY_APO)
            CHK_FAC(FACILITY_LEAP)
            CHK_FAC(FACILITY_WSAPI)

            default: StringCchCopy( strFacility, 100, TEXT("Unknown") ); break;
        }
        StringCchPrintf( strInternals, 100, TEXT("\r\nFacility Code: %s (%d)"), 
                    strFacility, HRESULT_FACILITY(hrErr) );
        StringCchCat( strHRESULTCopy, MAX_PATH*2, strInternals );

        StringCchPrintf( strInternals, 100, TEXT("\r\nError Code: 0x%0.4x (%lu)"), 
                    HRESULT_CODE(hrErr), HRESULT_CODE(hrErr) );
        StringCchCat( strHRESULTCopy, MAX_PATH*2, strInternals );

        SetDlgItemText( hDlg, IDC_MESSAGE, strHRESULTCopy );
    }

    return;
}
