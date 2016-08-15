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
#include "TextDlg.h"
#include "AppOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int CTextDlg::ID_TEXTE_CTRL = 100;
/////////////////////////////////////////////////////////////////////////////
// CTextDlg dialog


CTextDlg::CTextDlg(CWnd* pParent /*=NULL*/):
CDialog((TCHAR*)NULL, pParent)
{
	//{{AFX_DATA_INIT(CTextDlg)
	m_texte = _T("");
	//}}AFX_DATA_INIT
	

	
	/*m_hDialogTemplate = ::GlobalAlloc( GMEM_MOVEABLE, sizeof(DLGTEMPLATE) + 3*sizeof(WORD) );
	LPDLGTEMPLATE pDialogTemplate = (LPDLGTEMPLATE)::GlobalLock( m_hDialogTemplate );

	pDialogTemplate->style = WS_CHILD|WS_VISIBLE|DS_MODALFRAME;
    pDialogTemplate->dwExtendedStyle = NULL; 
    pDialogTemplate->cdit = 0; 
    pDialogTemplate->x = 0; 
    pDialogTemplate->y = 0; 
    pDialogTemplate->cx = 100; 
    pDialogTemplate->cy = 100; 

	WORD* pExtra = (LPWORD) (pDialogTemplate + sizeof(DLGTEMPLATE));
	pExtra[0] = NULL;
	pExtra[1] = NULL;
	pExtra[2] = NULL;
*/
	CRect rect(5,5,150,270);
	m_dlgItem[0].Initialize(CDialogItem::EDITCONTROL, rect, _T(""), ID_TEXTE_CTRL);

	// now initialize the DLGTEMPLATE structure
	m_dlgTempl.cx = 160;  // 4 horizontal units are the width of one character
	m_dlgTempl.cy = 280;  // 8 vertical units are the height of one character
	m_dlgTempl.style = WS_CAPTION | WS_VISIBLE | WS_THICKFRAME | WS_POPUP | DS_MODALFRAME | DS_SETFONT | WS_SYSMENU;//WS_DLGFRAME 
	m_dlgTempl.dwExtendedStyle = 0;
	m_dlgTempl.x = 0;
	m_dlgTempl.y = 0;
	m_dlgTempl.cdit = 1;  // 0 dialog items in the dialog

	InitTemplateHandle();
	//InitModalIndirect( pDialogTemplate , pParent);
}

void CTextDlg::InitTemplateHandle()
{
	// The first step is to allocate memory to define the dialog.  The information to be
	// stored in the allocated buffer is the following:
	//
	// 1.  DLGTEMPLATE structure
	// 2.    0x0000 (Word) indicating the dialog has no menu
	// 3.    0x0000 (Word) Let windows assign default class to the dialog
	// 4.    (Caption)  Null terminated unicode string
	// 5.    0x000B  (size of the font to be used)
	// 6.    "Arial"  (name of the typeface to be used)
	// 7.  DLGITEMTEMPLATE structure for the button  (HAS TO BE DWORD ALIGNED)
	// 8.    0x0080  to indicate the control is a button
	// 9.    (Title). Unicode null terminated string with the caption
	// 10.    0x0000   0 extra bytes of data for this control
	// 11.  DLGITEMTEMPLATE structure for the Static Text  (HAS TO BE DWORD ALIGNED)
	// 12.    0x0081 to indicate the control is static text
	// 13.   (Title). Unicode null terminated string with the text
	// 14     0x0000.  0 extra bytes of data for this control
	// 15. DLGITEMTEMPLATE structure for the Edit Control (HAS TO BE DWORD ALIGNED)
	// 16.   0x0082 to indicate an Edit control
	// 17.   (Text) - Null terminated unicode string to appear in the edit control
	// 18.   0x0000. 0 extra bytes of data for this control

	//WCHAR szBoxCaption[] = L"";
	m_strCaption = AfxGetAppName();
	WCHAR szFontName[] = L"MS Sans Sherif";

	// will first convert the control captions to UNICODE
	int     nTotalLength = 0;
//	int     i;

	TRY  // catch memory exceptions and don't worry about allocation failures
	{
		// The following expressions have unnecessary parenthesis trying to make the
		// comments more clear.
		int nBufferSize =  sizeof(DLGTEMPLATE) + (2 * sizeof(WORD))/*menu and class*/ ;//+ sizeof(szBoxCaption);
		nBufferSize += (m_strCaption.GetLength() + 1) * sizeof(WCHAR);
		nBufferSize += sizeof(WORD) + sizeof(szFontName); /* font information*/
		nBufferSize = ((nBufferSize + 3) & ~3);  // adjust size to make first control DWORD aligned

		for (int i = 0; i < m_dlgTempl.cdit; i++)
		{
			int nItemLength = sizeof(DLGITEMTEMPLATE) + 3 * sizeof(WORD);
			nItemLength += (m_dlgItem[i].m_strCaption.GetLength() + 1) * sizeof(WCHAR);

			if (i != m_dlgTempl.cdit -1 )   // the last control does not need extra bytes
				nItemLength = (nItemLength + 3) & ~3;  // take into account gap so next control is DWORD aligned

			nBufferSize += nItemLength;
		}


		m_hLocal = LocalAlloc(LHND, nBufferSize);
		if (m_hLocal == NULL)
			AfxThrowMemoryException();

		BYTE*   pBuffer = (BYTE*)LocalLock(m_hLocal);
		if (pBuffer == NULL)
		{
			LocalFree(m_hLocal);
			AfxThrowMemoryException();
		}

		BYTE*   pdest = pBuffer;
		// transfer DLGTEMPLATE structure to the buffer
		memcpy(pdest, &m_dlgTempl, sizeof(DLGTEMPLATE));
		pdest += sizeof(DLGTEMPLATE);
		*(WORD*)pdest = 0; // no menu
		*(WORD*)(pdest + 1) = 0;  // use default window class
		pdest += 2 * sizeof(WORD);

//		memcpy(pdest, szBoxCaption, sizeof(szBoxCaption));
//		pdest += sizeof(szBoxCaption);
			int nChars = m_strCaption.GetLength() + 1;
			WCHAR* pchCaption = new WCHAR[nChars];
			int nActualChars = MultiByteToWideChar(CP_ACP, 0, m_strCaption, -1, pchCaption, nChars);
			ASSERT(nActualChars > 0);
			memcpy(pdest, pchCaption, nActualChars * sizeof(WCHAR));
			pdest += nActualChars * sizeof(WCHAR);
			delete pchCaption;

		*(WORD*)pdest = 11;  // font size
		pdest += sizeof(WORD);
		memcpy(pdest, szFontName, sizeof(szFontName));
		pdest += sizeof(szFontName);

		// will now transfer the information for each one of the item templates
		for (int i = 0; i < m_dlgTempl.cdit; i++)
		{
			pdest = (BYTE*)(((DWORD)pdest + 3) & ~3);  // make the pointer DWORD aligned
			memcpy(pdest, (void *)&m_dlgItem[i].m_dlgItemTemplate, sizeof(DLGITEMTEMPLATE));
			pdest += sizeof(DLGITEMTEMPLATE);
			*(WORD*)pdest = 0xFFFF;  // indicating atom value
			pdest += sizeof(WORD);
			*(WORD*)pdest = m_dlgItem[i].m_controltype;    // atom value for the control
			pdest += sizeof(WORD);

			// transfer the caption even when it is an empty string
			WCHAR*  pchCaption;
			int     nChars, nActualChars;

			nChars = m_dlgItem[i].m_strCaption.GetLength() + 1;
			pchCaption = new WCHAR[nChars];
			nActualChars = MultiByteToWideChar(CP_ACP, 0, m_dlgItem[i].m_strCaption, -1, pchCaption, nChars);
			ASSERT(nActualChars > 0);
			memcpy(pdest, pchCaption, nActualChars * sizeof(WCHAR));
			pdest += nActualChars * sizeof(WCHAR);
			delete pchCaption;

			*(WORD*)pdest = 0;  // How many bytes in data for control
			pdest += sizeof(WORD);
		}
		
		ASSERT(pdest - pBuffer == nBufferSize); // just make sure we did not overrun the heap

		
		InitModalIndirect((DLGTEMPLATE*)pBuffer);
		//dlg.DoModal();  // tadaaa!  this is the line everyone's been waiting for!!!

		
	}
	CATCH(CMemoryException, e)
	{
		ASSERT(false);
		//TRACE(_T("Memory allocation for dialog template failed.  Demo aborted!"),
		//	_T("Allocation Failure"), MB_ICONEXCLAMATION | MB_OK);
	}
	END_CATCH
}

CTextDlg::~CTextDlg()
{
//	::GlobalUnlock( m_hDialogTemplate );
//	::GlobalFree( m_hDialogTemplate );
	LocalUnlock(m_hLocal);
	LocalFree(m_hLocal);
}


void CTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextDlg)
	DDX_Text(pDX, ID_TEXTE_CTRL, m_texte);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextDlg, CDialog)
	//{{AFX_MSG_MAP(CTextDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextDlg message handlers

void CTextDlg::OnOk() 
{
	CDialog::OnOK();
//    DestroyWindow();
}

void CTextDlg::OnCancel() 
{
	CDialog::OnCancel();
  //  DestroyWindow();
}

void CTextDlg::PostNcDestroy() 
{
	CDialog::PostNcDestroy();
//    delete this;
}

int CTextDlg::OnCreate( LPCREATESTRUCT lpcs)
{
	if( !CDialog::OnCreate( lpcs ) )
		return FALSE;


	return TRUE;
}

void CTextDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

    switch( nType )
	{
	case SIZE_MAXIMIZED:
	case SIZE_RESTORED:
		{
			if( &GetTextCtrl() != NULL && GetTextCtrl().m_hWnd != NULL)
			{
                CRect rectText;
				
                rectText.left = 8;
                rectText.top = 8;
                rectText.bottom = cy -10;
                rectText.right = cx-10;
                    
				GetTextCtrl().MoveWindow(rectText);
			}
		}

	case SIZE_MINIMIZED:
	case SIZE_MAXHIDE:
	case SIZE_MAXSHOW: break;
	default: ASSERT(false);
	}
	

}

BOOL CTextDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    //HICON hIcon = AfxGetApp()->LoadIcon(m_IdIcon);//remetter IDR_MAINFRAME dans biokrigig
    //SetIcon( hIcon, FALSE);

    CAppOption op;
    WINDOWPLACEMENT wndPl;
    if( op.GetWindowPlacement(_T("textDlg"), wndPl) )
        SetWindowPlacement(&wndPl);

    GetTextCtrl().SetSel(0,0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTextDlg::OnDestroy() 
{
    CAppOption op;

    WINDOWPLACEMENT wndPl;
    if( GetWindowPlacement(&wndPl) )
        op.SetWindowPlacement(_T("textDlg"), wndPl);


    CDialog::OnDestroy();
}

//*************************************************************************

void CDialogItem::Initialize(TControl ctrltype, CRect& rect, LPCTSTR lpszCaption, UINT nID)
{
	// first fill in the type, location and size of the control
	m_controltype = ctrltype;	
	if (rect != NULL)
	{
		// disable warning on conversion from long to short
		m_dlgItemTemplate.x = (short)rect.left;
		m_dlgItemTemplate.y = (short)rect.top;
		m_dlgItemTemplate.cx = (short)rect.Width();
		m_dlgItemTemplate.cy = (short)rect.Height();

	}
	
	m_dlgItemTemplate.dwExtendedStyle = 0;
	m_dlgItemTemplate.id = nID;

	// the styles below are hard coded.  In a real life application you might want to use variables to
	// define the sytle.
	switch(m_controltype)
	{
	case BUTTON:
		m_dlgItemTemplate.style = WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON;
		break;
	case EDITCONTROL:
		m_dlgItemTemplate.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_LEFT |WS_BORDER|WS_VSCROLL|WS_HSCROLL;
		break;
	case STATICTEXT:
		m_dlgItemTemplate.style = WS_CHILD | WS_VISIBLE | SS_LEFT;
		break;
	default:
		ASSERT(FALSE);  // should never get here, anyway.
	}

	m_strCaption = (lpszCaption != NULL)? lpszCaption : _T("");
}
