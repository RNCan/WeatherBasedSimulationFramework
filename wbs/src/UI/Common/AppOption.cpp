//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "UI/Common/AppOption.h" 


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAppOption::CAppOption(const CString& profile)
{
	m_currentProfile = profile;
//	if( profile.IsEmpty() )
//		m_currentProfile = "Settings";
//	else m_currentProfile = profile;


//	AfxGetApp() = AfxGetApp( ); 
  //  ASSERT(AfxGetApp());

//	m_appPath = GetCurrentAppPath();
}
	
CAppOption::~CAppOption()
{
}

/*CString CAppOption::GetApplicationPath()
{
	char buffer[MAX_PATH];

	GetModuleFileName(NULL, buffer, MAX_PATH);
	

	for(int i=strlen(buffer); i > 0; i--)
	{
		if (buffer[i-1] == '\\')
		{
			buffer[i] = '\0';
			break;
		}
	}

	return buffer;
}
*/

//***********************************************
CString CAppOption::GetLastFile()const
{	
	return AfxGetApp()->GetProfileString(m_currentProfile, _T("LastFile"), _T(""));
}
void CAppOption::SetLastFile(const CString& lastFile)
{
	AfxGetApp()->WriteProfileString(m_currentProfile, _T("LastFile"), lastFile);
}

//***********************************************

bool CAppOption::GetWindowPlacement(const CString& entryName, WINDOWPLACEMENT& wnd)const
{
	TCHAR szFormat[] = _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d");

	CString buffer = AfxGetApp()->GetProfileString(m_currentProfile, entryName, _T("") );

	if (buffer.IsEmpty())
		return false;

	int nRead = _stscanf_s(buffer, szFormat,
		&wnd.length,
		&wnd.flags,
		&wnd.showCmd,
		&wnd.ptMinPosition.x,
		&wnd.ptMinPosition.y,
		&wnd.ptMaxPosition.x,
		&wnd.ptMaxPosition.y,
		&wnd.rcNormalPosition.left,
		&wnd.rcNormalPosition.top,
		&wnd.rcNormalPosition.right,
		&wnd.rcNormalPosition.bottom);

	if (nRead != 11 || wnd.length != sizeof(WINDOWPLACEMENT) )
		return false;

	return true;
}

void CAppOption::SetWindowPlacement(const CString& entryName, const WINDOWPLACEMENT& wnd )const
{
	TCHAR szFormat[] = _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d");
    CString buffer;

	buffer.Format(szFormat,
		wnd.length,
		wnd.flags,
		wnd.showCmd,
		wnd.ptMinPosition.x,
		wnd.ptMinPosition.y,
		wnd.ptMaxPosition.x,
		wnd.ptMaxPosition.y,
		wnd.rcNormalPosition.left,
		wnd.rcNormalPosition.top,
		wnd.rcNormalPosition.right,
		wnd.rcNormalPosition.bottom);

    AfxGetApp()->WriteProfileString( m_currentProfile, entryName, buffer);

	/*
	TCHAR szFormat[] = _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");
	TCHAR szBuffer[sizeof("-32767")*8 + sizeof("65535")*2];

	wsprintf(szBuffer, szFormat,
		pwp->flags, pwp->showCmd,
		pwp->ptMinPosition.x, pwp->ptMinPosition.y,
		pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
		pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
		pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
	AfxGetApp()->WriteProfileString(szSection, szWindowPos, szBuffer);
*/
}

//***********************************************
bool CAppOption::GetOpenLastFile()
{
	return (AfxGetApp()->GetProfileInt(m_currentProfile, _T("Open last file"), 0) ? true : false);
}

void CAppOption::SetOpenLastFile(bool lastFile)
{
	AfxGetApp()->WriteProfileInt(m_currentProfile, _T("Open last file"), lastFile);
}

//***********************************************
void CAppOption::GetLastOpenPath(const CString& subject, CString& path)const
{
    if( path.IsEmpty() )
    {
        wchar_t defaultPath [MAX_PATH];
        VERIFY( GetCurrentDirectoryW(MAX_PATH, defaultPath ) );
        path = defaultPath;
    }

	path = AfxGetApp()->GetProfileString(_T("Last open path"), subject, path);
}

void CAppOption::SetLastOpenPath(const CString& subject, const CString& path )
{
	AfxGetApp()->WriteProfileString(_T("Last open path"), subject, path);
}

//***********************************************


TCHAR CAppOption::szHeight[] = _T("Height");
TCHAR CAppOption::szWeight[] = _T("Weight");
TCHAR CAppOption::szItalic[] = _T("Italic");
TCHAR CAppOption::szUnderline[] = _T("Underline");
TCHAR CAppOption::szPitchAndFamily[] = _T("PitchAndFamily");
TCHAR CAppOption::szCharSet[] = _T("CharSet");
TCHAR CAppOption::szFaceName[] = _T("FaceName");
TCHAR CAppOption::szCourier[] = _T("Courier New");


void CAppOption::GetFont(LPCTSTR szSec, LOGFONT* plf)
{
//	CWinApp* AfxGetApp() = AfxGetApp();
    CClientDC dc(AfxGetMainWnd());
    int nHeigth = -((dc.GetDeviceCaps(LOGPIXELSY)*10)/72);
	
    plf->lfHeight = AfxGetApp()->GetProfileInt(szSec, szHeight, nHeigth);
	if (plf->lfHeight != 0)
	{
		plf->lfWeight = AfxGetApp()->GetProfileInt(szSec, szWeight, nHeigth );
        
        plf->lfWidth = 0;
        plf->lfEscapement = 0;    
        plf->lfOrientation = 0; 
		plf->lfItalic = (BYTE)AfxGetApp()->GetProfileInt(szSec, szItalic, 0);
		plf->lfUnderline = (BYTE)AfxGetApp()->GetProfileInt(szSec, szUnderline, 0);
        plf->lfStrikeOut = 0;
        plf->lfCharSet = DEFAULT_CHARSET;
        plf->lfOutPrecision = OUT_CHARACTER_PRECIS;
        plf->lfClipPrecision = CLIP_CHARACTER_PRECIS; 
        plf->lfQuality = DEFAULT_QUALITY; 

		plf->lfPitchAndFamily = (BYTE)AfxGetApp()->GetProfileInt(szSec, szPitchAndFamily, DEFAULT_PITCH|FF_DONTCARE);
		plf->lfCharSet = (BYTE)AfxGetApp()->GetProfileInt(szSec, szCharSet, DEFAULT_CHARSET);
		CString strFont = AfxGetApp()->GetProfileString(szSec, szFaceName, szCourier);
		lstrcpyn((TCHAR*)plf->lfFaceName, strFont, sizeof plf->lfFaceName);
		plf->lfFaceName[sizeof plf->lfFaceName-1] = 0;
	}
}

void CAppOption::SetFont(LPCTSTR szSec, const LOGFONT* plf)
{
//WinApp* AfxGetApp() = AfxGetApp();

	AfxGetApp()->WriteProfileInt(szSec, szHeight, plf->lfHeight);
	if (plf->lfHeight != 0)
	{
		AfxGetApp()->WriteProfileInt(szSec, szHeight, plf->lfHeight);
		AfxGetApp()->WriteProfileInt(szSec, szWeight, plf->lfWeight);
		AfxGetApp()->WriteProfileInt(szSec, szItalic, plf->lfItalic);
		AfxGetApp()->WriteProfileInt(szSec, szUnderline, plf->lfUnderline);
		AfxGetApp()->WriteProfileInt(szSec, szPitchAndFamily, plf->lfPitchAndFamily);
		AfxGetApp()->WriteProfileInt(szSec, szCharSet, plf->lfCharSet);
		AfxGetApp()->WriteProfileString(szSec, szFaceName, (LPCTSTR)plf->lfFaceName);
	}

}


void CAppOption::WriteProfileInt(const CString& entryName, int nValue)
{
    AfxGetApp()->WriteProfileInt(m_currentProfile, entryName, nValue);
}
int CAppOption::GetProfileInt(const CString& entryName, int nDefault)
{
    return AfxGetApp()->GetProfileInt(m_currentProfile, entryName, nDefault);
}

void CAppOption::WriteProfileFloat( const CString& entryName, float fValue)
{
    WriteProfileDouble( entryName, fValue);
}

float CAppOption::GetProfileFloat( const CString& entryName, float fDefault)
{
    return (float)GetProfileDouble( entryName, fDefault);
}

void CAppOption::WriteProfileDouble( const CString& entryName, double fValue)
{
    CString tmp;
	tmp.Format(_T("%.18g"), fValue);
    AfxGetApp()->WriteProfileString(m_currentProfile, entryName, tmp);
}


double CAppOption::GetProfileDouble( const CString& entryName, double fDefault)
{
    CString tmp;
	tmp.Format(_T("%.18lg"), fDefault);

    tmp = AfxGetApp()->GetProfileString(m_currentProfile, entryName, tmp);
    double fValue=0;
	swscanf_s((LPCTSTR)tmp, _T("%lf"), &fValue);
    
    return fValue;
}





void CAppOption::WriteProfileBool(const CString& entryName, bool bValue)
{
    AfxGetApp()->WriteProfileInt(m_currentProfile, entryName, bValue?TRUE:FALSE);
}
bool CAppOption::GetProfileBool(const CString& entryName, bool bDefault)
{
    return AfxGetApp()->GetProfileInt(m_currentProfile, entryName, bDefault?TRUE:FALSE)?true:false;
}

void CAppOption::WriteProfileString(const CString& entryName, CString string)
{
    AfxGetApp()->WriteProfileString(m_currentProfile, entryName, string);
}

CString CAppOption::GetProfileString(const CString& entryName, CString string)
{
    return AfxGetApp()->GetProfileString(m_currentProfile, entryName, string);
}

void CAppOption::WriteProfileCOLORREF(const CString& entryName, COLORREF color)
{
    AfxGetApp()->WriteProfileInt(m_currentProfile, entryName, (int)color);
}

COLORREF CAppOption::GetProfileCOLORREF(const CString& entryName, COLORREF nDefault)
{
    return (COLORREF)AfxGetApp()->GetProfileInt(m_currentProfile, entryName, (int)nDefault);
}

CPoint CAppOption::GetProfilePoint(const CString& entryName, CPoint nDefault)
{
	CPoint pt;

	CString ptStr = GetProfileString(entryName, _T("") );
	if( !ptStr.IsEmpty() )
	{
		swscanf_s(ptStr, _T("%d %d"), &pt.x, &pt.y);
	}
	else pt = nDefault;

	return pt;
}

void CAppOption::WriteProfilePoint(const CString& entryName, CPoint pt)
{
	CString ptStr;
	ptStr.Format(_T("%d %d"), pt.x, pt.y);

	WriteProfileString( entryName, ptStr);
}

CRect CAppOption::GetProfileRect(const CString& entryName, CRect nDefault)
{
	CRect rect;

	CString ptStr = GetProfileString(entryName, _T(""));
	if( !ptStr.IsEmpty() )
	{
		swscanf_s(ptStr, _T("%d %d %d %d"), &rect.left, &rect.top, &rect.right, &rect.bottom);
	}
	else rect = nDefault;

	return rect;
}

void CAppOption::WriteProfileRect(const CString& entryName, const CRect& rect)
{
	CString ptStr;
	ptStr.Format(_T("%d %d %d %d"), rect.left, rect.top, rect.right, rect.bottom);

	WriteProfileString( entryName, ptStr);
}
/*void CAppOption::GetProfileGeoRect(const CString& entryName, CGeoRect& rect)
{
	rect.m_north() = GetProfileDouble(entryName + "(north)", rect.m_north());
	rect.m_south() = GetProfileDouble(entryName + "(south)", rect.m_south());
	rect.m_east() = GetProfileDouble(entryName + "(east)", rect.m_east());
	rect.m_west() = GetProfileDouble(entryName + "(west)", rect.m_west());
	rect.SetPrjType(GetProfileInt(entryName + "(PrjType)", rect.GetPrjType()));
}

void CAppOption::WriteProfileGeoRect(const CString& entryName, const CGeoRect& rect )
{

	WriteProfileDouble(entryName + "(north)", rect.m_north());
	WriteProfileDouble(entryName + "(south)", rect.m_south());
	WriteProfileDouble(entryName + "(east)", rect.m_east());
	WriteProfileDouble(entryName + "(west)", rect.m_west());
	WriteProfileInt(entryName + "(PrjType)", rect.GetPrjType());
}
*/