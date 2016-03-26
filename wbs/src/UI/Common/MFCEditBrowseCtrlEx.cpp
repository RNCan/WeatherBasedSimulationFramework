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
#include "MFCEditBrowseCtrlEx.h"

#include "WeatherBasedSimulationUI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CMFCEditBrowseCtrl

CMFCEditBrowseCtrlEx::CMFCEditBrowseCtrlEx()
{
}

CMFCEditBrowseCtrlEx::~CMFCEditBrowseCtrlEx()
{
}

BEGIN_MESSAGE_MAP(CMFCEditBrowseCtrlEx, CMFCEditBrowseCtrl)
	//{{AFX_MSG_MAP(CMFCEditBrowseCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCEditBrowseCtrl message handlers

void CMFCEditBrowseCtrlEx::OnBrowse()
{
	ASSERT_VALID(this);
	ENSURE(GetSafeHwnd() != NULL);

	switch (m_Mode)
	{
	case BrowseMode_Folder:
		//if (afxShellManager != NULL)
		{
			CString strFolder = GetWindowText();

			CString strResult;
			
			//BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
			m_ulBrowseFolderFlags |= BIF_NEWDIALOGSTYLE;
			afxShellManager->BrowseForFolder(strResult, this, strFolder);
			if(strResult != strFolder)
			//CXFolderDialog dlg(strFolder, 0, this);
			//dlg.SetFilter("AllFile(*.*)|*.*||");
			//dlg.EnableRegistry(TRUE);

			
			//if ( (dlg.DoModal() == IDOK) )
			{
				//if(dlg.GetPath() != strFolder)
				//{
					//SetWindowText(dlg.GetPath());
					SetWindowText(strResult);
					
					SetModify(TRUE);
					OnAfterUpdate();
				//}
			}
		}
		//else
		//{
			//ASSERT(FALSE);
		//}
		break;

	case BrowseMode_File:
		{
			CString strFile = GetWindowText();

			if (!strFile.IsEmpty())
			{
				TCHAR fname [_MAX_FNAME];

				_tsplitpath_s(strFile, NULL, 0, NULL, 0, fname, _MAX_FNAME, NULL, 0);

				CString strFileName = fname;
				strFileName.TrimLeft();
				strFileName.TrimRight();

				if (strFileName.IsEmpty())
				{
					strFile.Empty();
				}
			}

			
			CFileDialog dlg(TRUE, !m_strDefFileExt.IsEmpty() ? (LPCTSTR)m_strDefFileExt : (LPCTSTR)NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, !m_strFileFilter.IsEmpty() ? (LPCTSTR)m_strFileFilter : (LPCTSTR)NULL, NULL);
			dlg.GetOFN().nFilterIndex = m_nFilterIndex;
			if (dlg.DoModal() == IDOK && strFile != dlg.GetPathName())
			{
				SetWindowText(dlg.GetPathName());
				SetModify(TRUE);
				OnAfterUpdate();
			}

			m_nFilterIndex = dlg.GetOFN().nFilterIndex;
			if( !m_defaultEntry.IsEmpty() )
				AfxGetApp()->WriteProfileInt(_T("FileDialog Filter index"), m_defaultEntry, m_nFilterIndex);

			if (GetParent() != NULL)
			{
				GetParent()->RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
			}
		}
		break;
	}

	SetFocus();
}


void CMFCEditBrowseCtrlEx::EnableFileBrowseButton(LPCTSTR lpszDefExt, LPCTSTR lpszFilter, LPCTSTR defaultEntry, int nDefaultFilter)
{
	CMFCEditBrowseCtrl::EnableFileBrowseButton(lpszDefExt, lpszFilter);
	HICON hIcon = (HICON)::LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_CMN_OPEN), IMAGE_ICON, 16, 16, LR_SHARED);
	SetBrowseButtonImage(hIcon, true);

	m_defaultEntry = defaultEntry;
	ASSERT( !m_defaultEntry.IsEmpty() );

	if( !m_defaultEntry.IsEmpty() )
		m_nFilterIndex = AfxGetApp()->GetProfileInt(_T("FileDialog Filter index"), m_defaultEntry, nDefaultFilter);
}

void CMFCEditBrowseCtrlEx::EnableFileBrowseButton(LPCTSTR lpszDefExt, LPCTSTR lpszFilter, int nFilterIndex)
{
	CMFCEditBrowseCtrl::EnableFileBrowseButton(lpszDefExt, lpszFilter);
	HICON hIcon = (HICON)::LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_CMN_OPEN), IMAGE_ICON, 16, 16, LR_SHARED);
	SetBrowseButtonImage(hIcon, true);
	m_nFilterIndex = nFilterIndex;
}

void CMFCEditBrowseCtrlEx::EnableFolderBrowseButton()
{
	CMFCEditBrowseCtrl::EnableFolderBrowseButton();
	HICON hIcon = (HICON)::LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_CMN_OPEN), IMAGE_ICON, 16, 16, LR_SHARED);
	SetBrowseButtonImage(hIcon, true);
}

/*
void DDX_FileEditCtrl(CDataExchange *pDX, int nIDC, CString& rStr, DWORD dwFlags)
{
    CWnd *pWnd = pDX->m_pDlgWnd->GetDlgItem(nIDC);
    ASSERT(pWnd);
    if (pDX->m_bSaveAndValidate)                // update string with text from control
    {
        // ensure the control is a CFileEditCtrl control
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)));
        // copy the first file listed in the control to the string
        rStr.Empty();
        CFileEditCtrl *pFEC = (CFileEditCtrl *)pWnd;
        POSITION pos = pFEC->GetStartPosition();
        if (pos)
            rStr = pFEC->GetNextPathName(pos);
    }
    else                                        // create the control if it is not already created
    {                                           // set the control text to the text in string
        CFileEditCtrl *pFEC = NULL;
        if (!pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)))    // not subclassed yet
        {
            // create then subclass the control to the edit control with the ID nIDC
            HWND hWnd = pDX->PrepareEditCtrl(nIDC);
            pFEC = new CFileEditCtrl(TRUE);     // create the control with autodelete
            if (!pFEC)                          // failed to create control
            {
                ASSERT(FALSE);
                AfxThrowNotSupportedException();
            }
            if (!pFEC->SubclassWindow(hWnd))
            {                                   // failed to subclass the edit control
                delete pFEC;
                ASSERT(FALSE);
                AfxThrowNotSupportedException();
            }
            // call CFileEditCtrl::SetFlags() to initialize the internal data structures
            dwFlags &= ~FEC_MULTIPLE;           // can not put multiple files in one CString
            if (!pFEC->SetFlags(dwFlags))
            {
                delete pFEC;
                ASSERT(FALSE);
                AfxThrowNotSupportedException();
            }
        }
        else                                    // control already exists
            pFEC = (CFileEditCtrl *)pWnd;
        if (pFEC)
            pFEC->SetWindowText(rStr);          // set the control text
    }
}
*/