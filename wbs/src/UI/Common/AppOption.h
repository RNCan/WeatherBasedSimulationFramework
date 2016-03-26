//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


class CAppOption//: public WBSF::CRegistry
{
public:
	CAppOption(const CString& profile=_T("Settings"));
	virtual ~CAppOption();

	//const CString& GetApplicationPath()const{return m_appPath;}

	const CString& GetCurrentProfile(){return m_currentProfile;}
	void SetCurrentProfile(const CString& profile=_T("Settings")){m_currentProfile = profile;}

    void WriteProfileInt(const CString& entryName, int nValue);
    int GetProfileInt(const CString& entryName, int nDefault=0);
    void WriteProfileFloat( const CString& entryName, float fValue);
    float GetProfileFloat( const CString& entryName, float fDefault=0);
    void WriteProfileDouble( const CString& entryName, double fValue);
    double GetProfileDouble( const CString& entryName, double fDefault=0);
    void WriteProfileBool(const CString& entryName, bool bValue);
    bool GetProfileBool(const CString& entryName, bool bDefault=false);
    void WriteProfileString(const CString& entryName, CString string);
    CString GetProfileString(const CString& entryName, CString string=_T(""));
    void WriteProfileCOLORREF(const CString& entryName, COLORREF color);
    COLORREF GetProfileCOLORREF(const CString& entryName, COLORREF nDefault=RGB(0,0,0));
	CPoint GetProfilePoint(const CString& entryName, CPoint nDefault=CPoint(0,0));
	void WriteProfilePoint(const CString& entryName, CPoint point);
	CRect GetProfileRect(const CString& entryName, CRect nDefault=CRect(0,0,0,0));
	void WriteProfileRect(const CString& entryName, const CRect& rect);

//	void GetProfileGeoRect(const CString& entryName, CGeoRect& rect);
//	void WriteProfileGeoRect(const CString& entryName, const CGeoRect& rect );

    bool GetOpenLastFile();
	void SetOpenLastFile(bool lastFile);
    CString GetLastFile()const;
    void SetLastFile(const CString& lastFile);

    bool GetWindowPlacement(const CString& entryName, WINDOWPLACEMENT& wnd)const;
	void SetWindowPlacement(const CString& entryName, const WINDOWPLACEMENT& wnd )const;

    void GetLastOpenPath(const CString& subject, CString& path)const;
    void SetLastOpenPath(const CString& subject, const CString& path);

    void GetFont(LPCTSTR szSec, LOGFONT* plf);
    void SetFont(LPCTSTR szSec, const LOGFONT* plf);

protected:

	CString m_appPath;

private:

    
    static TCHAR szHeight[];
    static TCHAR szWeight[];
    static TCHAR szItalic[];
    static TCHAR szUnderline[];
    static TCHAR szPitchAndFamily[];
    static TCHAR szCharSet[];
    static TCHAR szFaceName[];
    static TCHAR szCourier[];
    
    CString m_currentProfile;
};

