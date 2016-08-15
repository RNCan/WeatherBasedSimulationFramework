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

#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "WeaLinkDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{
	/////////////////////////////////////////////////////////////////////////////
	// CWeaLinkDlg dialog


	CWeaLinkDlg::CWeaLinkDlg(CDailyDatabase& dailyDB, CWnd* pParent /*=NULL*/) :
		CDialog(CWeaLinkDlg::IDD, pParent),
		//m_fileManager(fileManager),
		m_dailyDB(dailyDB)
	{
		//{{AFX_DATA_INIT(CWeaLinkDlg)
		// NOTE: the ClassWizard will add member initialization here
		//}}AFX_DATA_INIT
	}


	void CWeaLinkDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CWeaLinkDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
		//}}AFX_DATA_MAP
	}


	BEGIN_MESSAGE_MAP(CWeaLinkDlg, CDialog)
		//{{AFX_MSG_MAP(CWeaLinkDlg)
		// NOTE: the ClassWizard will add msg map macros here
		//}}AFX_MSG_MAP
		ON_BN_CLICKED(IDC_RT_ADD_LINK, OnAddLink)
		ON_BN_CLICKED(IDC_RT_REMOVE_LINK, OnRemoveLink)
		ON_LBN_SELCHANGE(IDC_RT_UNLINKED, UpdateCtrl)
		ON_LBN_SELCHANGE(IDC_RT_LINKED, UpdateCtrl)
		ON_BN_CLICKED(IDC_RT_IMPORT, OnImport)
		ON_LBN_DBLCLK(IDC_RT_LINKED, OnRemoveLink)
		ON_LBN_DBLCLK(IDC_RT_UNLINKED, OnAddLink)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CWeaLinkDlg msg handlers

	BOOL CWeaLinkDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();


		for (int i = 0; i < m_packageNameList.GetSize(); i++)
			GetLinkedListCtrl().AddString(m_packageNameList[i]);

		//	CStringArray unlinkedFile;
		//	m_dailyDB.GetUnlinkedFile(unlinkedFile);

		for (int i = 0; i < m_unlinkedFile.GetSize(); i++)
		{
			//add unlinked file that is not in the current station
			//if( UtilWin::FindStringExact( m_packageNameList, m_unlinkedFile[i], false) == -1)
			GetUnlinkedListCtrl().AddString(m_unlinkedFile[i]);
		}

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CWeaLinkDlg::OnOK()
	{
		m_packageNameList.SetSize(GetLinkedListCtrl().GetCount());
		for (int i = 0; i < GetLinkedListCtrl().GetCount(); i++)
		{
			GetLinkedListCtrl().GetText(i, m_packageNameList[i]);
		}


		m_unlinkedFile.SetSize(GetUnlinkedListCtrl().GetCount());
		for (int i = 0; i < GetUnlinkedListCtrl().GetCount(); i++)
		{
			GetUnlinkedListCtrl().GetText(i, m_unlinkedFile[i]);
		}
		CDialog::OnOK();
	}


	void CWeaLinkDlg::OnAddLink()
	{

		int nSelCount = GetUnlinkedListCtrl().GetSelCount();
		int* pSel = new int[nSelCount];
		GetUnlinkedListCtrl().GetSelItems(nSelCount, pSel);


		for (int i = nSelCount - 1; i >= 0; i--)
		{
			MoveString(GetUnlinkedListCtrl(), GetLinkedListCtrl(), pSel[i]);
		}

		if (nSelCount > 0)
		{
			if (pSel[0] < GetLinkedListCtrl().GetCount())
				GetUnlinkedListCtrl().SetSel(pSel[0]);
			else GetUnlinkedListCtrl().SetSel(pSel[0] - 1);
		}

		delete pSel;


		UpdateCtrl();
	}

	void CWeaLinkDlg::OnRemoveLink()
	{
		int nSelCount = GetLinkedListCtrl().GetSelCount();
		int* pSel = new int[nSelCount];
		GetLinkedListCtrl().GetSelItems(nSelCount, pSel);


		for (int i = nSelCount - 1; i >= 0; i--)
		{
			MoveString(GetLinkedListCtrl(), GetUnlinkedListCtrl(), pSel[i]);
		}

		if (nSelCount > 0)
		{
			if (pSel[0] < GetLinkedListCtrl().GetCount())
				GetLinkedListCtrl().SetSel(pSel[0]);
			else GetLinkedListCtrl().SetSel(pSel[0] - 1);
		}

		delete pSel;


		UpdateCtrl();

	}

	void CWeaLinkDlg::MoveString(CListBox& lisboxFrom, CListBox& lisboxTo, int posFrom)
	{
		CString text;
		lisboxFrom.GetText(posFrom, text);
		DWORD data = (int)lisboxFrom.GetItemData(posFrom);

		lisboxFrom.DeleteString(posFrom);

		int nCount = lisboxFrom.GetCount();
		if (nCount > 0)
		{
			if (posFrom == nCount)lisboxFrom.SetCurSel(posFrom - 1);
			else lisboxFrom.SetCurSel(posFrom);
		}
		else lisboxFrom.SetCurSel(-1);


		int pos = lisboxTo.InsertString(0, text);
		lisboxTo.SetItemData(pos, data);
		lisboxTo.SetSel(pos, TRUE);
		//lisboxTo.SetCurSel(pos);
		//lisboxTo.SelItemRange(true, pos, pos);

	}
	/*
	void CWeaLinkDlg::MoveString( CListBox& lisbox, int oldPos, int newPos )
	{
	CString text;
	lisbox.GetText( oldPos, text );
	DWORD data = lisbox.GetItemData(oldPos);

	lisbox.DeleteString( oldPos );
	lisbox.InsertString( newPos, text );
	lisbox.SetItemData(newPos, data);
	lisbox.SetCurSel(newPos);
	}
	*/
	void CWeaLinkDlg::SetSelection(CListBox& list, int* pSel, int nSize, int offset)
	{
		ASSERT(nSize > 0);

		for (int i = 0; i < nSize; i++)
			if (pSel[i] + offset >= 0 && pSel[i] + offset < list.GetCount())
				list.SetSel(pSel[i] + offset, TRUE);
	}


	void CWeaLinkDlg::UpdateCtrl()
	{

		bool bEnableAddAnalise = GetUnlinkedListCtrl().GetSelCount() > 0;
		bool bEnableRemoveAnalise = GetLinkedListCtrl().GetSelCount() > 0;
		//bool bEnableAnaliseUp = bEnableRemoveAnalise && GetLinkedListCtrl().GetSel(0) == 0;
		//bool bEnableAnaliseDown = bEnableRemoveAnalise && GetLinkedListCtrl().GetSel(GetLinkedListCtrl().GetCount()-1) == 0;

		GetAddCtrl().EnableWindow(bEnableAddAnalise);
		GetRemoveCtrl().EnableWindow(bEnableRemoveAnalise);
		//m_upAnalyseCtrl.EnableWindow(bEnableAnaliseUp);
		//m_downAnalyseCtrl.EnableWindow(bEnableAnaliseDown);
	}


	/*void CWeaLinkDlg::OnUpLink()
	{
	int nSelCount = GetLinkedListCtrl().GetSelCount();
	ASSERT(nSelCount != 0);

	int* pSel = new int[nSelCount];
	GetLinkedListCtrl().GetSelItems(nSelCount, pSel);


	for(int i=0; i< nSelCount; i++)
	{
	ASSERT( pSel[i] !=  0);
	MoveString( GetLinkedListCtrl(), pSel[i], pSel[i]-1);
	}

	SetSelection(GetLinkedListCtrl(), pSel, nSelCount, -1);

	delete pSel;

	UpdateCtrl();
	}

	void CWeaLinkDlg::OnDownLink()
	{
	int nSelCount = GetLinkedListCtrl().GetSelCount( );
	ASSERT(nSelCount != 0);

	int* pSel = new int[nSelCount];
	GetLinkedListCtrl().GetSelItems(nSelCount, pSel);


	for(int i=nSelCount-1; i>= 0; i--)
	{
	ASSERT(pSel[i] != GetLinkedListCtrl().GetCount()-1 );
	MoveString( GetLinkedListCtrl(), pSel[i], pSel[i]+1);
	}

	SetSelection(GetLinkedListCtrl(), pSel, nSelCount, 1);

	delete pSel;
	UpdateCtrl();
	}
	*/


	void CWeaLinkDlg::OnImport()
	{
		CFileDialog openDialog(true, _T(""), _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Daily data file (*.wea)|*.wea|All files(*.*)|*.*||"), this);
		openDialog.m_ofn.lpstrTitle = _T("Import");
		//openDialog.m_ofn.lpstrInitialDir = m_fileManager.GetWeatherPath();

		if (openDialog.DoModal() == IDOK)
		{
			ERMsg msg = ImportWEAFile(openDialog.GetPathName());

			if (msg)
			{
				GetLinkedListCtrl().AddString(openDialog.GetFileTitle());
			}
			else
			{
				UtilWin::SYShowMessage(msg, this);
			}
		}
		//copy du fichier dans le répertoire RealTime
		//	if( RTPath != path)
		//	{
		//		CString sOutMessage;
		//		AfxFormatString2(sOutMessage, IDS_RT_COPYTOIMPORT, openDialog.GetFileName(), m_fileManager.GetDailyPath());
		//				
		//		if( MessageBox(sOutMessage, AfxGetAppName() , MB_ICONQUESTION|MB_YESNO) == IDYES)
		//		{
		//			CString destFile =  m_fileManager.GetDailyPath()+openDialog.GetFileName();
		//			CFileStatus status;
		//			if( CFile::GetStatus(destFile , status) )
		//			{
		//				AfxFormatString1(sOutMessage, IDS_RT_ERRFILEEXIST, destFile);
		//				if( MessageBox(sOutMessage, AfxGetAppName() , MB_ICONQUESTION|MB_YESNOCANCEL) == IDYES)
		//				{
		//					if( !CopyFile(openDialog.GetPathName(),destFile, false) )
		//					{
		//						AfxFormatString1(sOutMessage, IDS_RT_ERRORTOCOPY, destFile);
		//						MessageBox(sOutMessage, AfxGetAppName() , MB_ICONEXCLAMATION|MB_OK);
		//						return;
		//					}
		//				}
		//				else return;

		//			}
		//			else if( !CopyFile(openDialog.GetPathName(),destFile, true) )
		//			{
		//				AfxFormatString1(sOutMessage, IDS_RT_ERRORTOCOPY, destFile);
		//				MessageBox(sOutMessage, AfxGetAppName() , MB_ICONEXCLAMATION|MB_OK);
		//				return;
		//			}
		//		}
		//		else return;

		//	}

		//	
		//	if (m_fileManager.GetImportRTStations(openDialog.GetFileName(), m_stationName, stationArray) )
		//	{
		//		//CIntArray yearAlreadyExist;
		//		CString badYear;

		//		int nSize = stationArray.GetSize();

		//		for(int i=0; i<nSize; i++)
		//		{
		//			CString newYearStr;
		//			newYearStr.Format("%4d",stationArray[i].GetYear() );

		//			if( m_yearList.FindStringExact(-1, newYearStr) == LB_ERR)
		//			{
		//				GetHeader(stationArray[i]);
		//				m_currentYear = stationArray[i].GetYear();

		//				FirstSaveData(stationArray[i]);	//save data to disk
		//			
		//			}
		//			else // the file already exist
		//			{
		//				badYear += newYearStr + ", ";
		//				//yearAlreadyExist.Add(stationArray[i].m_year);
		//			}
		//		}
		//		
		//		if(badYear.GetLength() >= 2)
		//		{
		//			badYear = badYear.Left( badYear.GetLength()-2);

		//			CString sOutMessage;
		//			AfxFormatString2(sOutMessage, IDS_YEARSEXIST, badYear, m_stationName);
		//				
		//			MessageBox(sOutMessage, AfxGetAppName() , MB_ICONEXCLAMATION|MB_OK);
		//		}

		//		//reconstruire la liste
		//		m_fileManager.GetRTDescList(m_descArray);
		//		UpdateButton();
		//	}
		//	else
		//	{
		//		CString sOutMessage;
		//		AfxFormatString1(sOutMessage, IDS_RT_CANNOTIMPORT, openDialog.GetFileName());
		//		
		//		MessageBox(sOutMessage, NULL , MB_ICONSTOP|MB_OK);

		//	}
		//}	
	}

	ERMsg CWeaLinkDlg::ImportWEAFile(const CString filePathIn)
	{
		ERMsg msg;
		ASSERT(false); //TODO
		/*
		if( UtilWin::GetPath( filePathIn ).CompareNoCase( m_dailyDB.GetDataPath() )!=0 )
		{
		VERIFY( UtilWin::CreateMultipleDir(m_dailyDB.GetDataPath() ));

		CString filePathOut = m_dailyDB.GetDataPath() + UtilWin::GetFileTitle(filePathIn) + ".wea";
		if( !CopyFile(filePathIn, filePathOut, true)  )
		{
		CString error;
		error.FormatMessage( IDS_DB_WEA_EXIST, UtilWin::GetFileTitle(filePathIn) );
		msg.asgType(ERMsg::ERREUR);
		msg.ajoute(error);
		}
		}
		*/

		return msg;
	}

}