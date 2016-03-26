«SXZ//******************************************************************************
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
#include "FileManager/FileManager.h"

#include "UI/OptionDir.h"
#include "UI/Common/AppOption.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	IMPLEMENT_DYNAMIC(CPathListBox, CVSListBox)
		void CPathListBox::EnableBrowse()
	{
		//m_wndEdit.EnableFolderBrowseButton(L"Browse path", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	}

	void CPathListBox::DoDataExchange(CDataExchange* pDX)
	{
	}

	// near the bottom of the file 
	void CPathListBox::OnBrowse()
	{
		CString strFolder;

		// for which item we started an browse
		int nSel = GetSelItem();
		ASSERT(nSel < GetCount()); // New item

		if (nSel < GetCount()) // New item
			strFolder = GetItemText(nSel);

		CString strResult;

		::OleInitialize(0);
		if (afxShellManager->BrowseForFolder(strResult, this, strFolder, L"Browse path", BIF_BROWSEINCLUDEFILES | BIF_NEWDIALOGSTYLE | BIF_UAHINT))
		{
			if (strResult != strFolder)
			{
				if (!UtilWin::DirExist(strResult))
				{
					strResult = UtilWin::GetPath(strResult);
				}
				//If we edit an item after the last one it must be a new one
				if (nSel == GetCount()) // New item
				{
					int nSel = AddItem(strResult);
					SelectItem(nSel);
				}
				else
				{
					SetItemText(nSel, strResult);
				}
			}
		}


		::OleUninitialize();
	}
	BEGIN_MESSAGE_MAP(CPathListBox, CVSListBox)
	END_MESSAGE_MAP()


	/////////////////////////////////////////////////////////////////////////////
	// COptionDir property page

	COptionDir::COptionDir() :
		CMFCPropertyPage(COptionDir::IDD)
	{

	}

	COptionDir::~COptionDir()
	{
	}

	void COptionDir::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_CMN_OPTION_APPPATH, m_appPathCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_MODELPATH, m_modelCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_WEATHER_PATH, m_weatherPathCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_MAP_PATH, m_mapPathCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_MAP_EXT, m_mapExtCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_PROJECT_WEATHER_PATH, m_projectWeatherPathCtrl);
		DDX_Control(pDX, IDC_CMN_OPTION_PROJECT_MAP_PATH, m_projectMapPathCtrl);
	}


	BEGIN_MESSAGE_MAP(COptionDir, CMFCPropertyPage)
	END_MESSAGE_MAP()


	BOOL COptionDir::OnInitDialog()
	{
		CMFCPropertyPage::OnInitDialog();

		//m_weatherPathCtrl.m_pWndList->DeleteAllItems();
		//m_mapPathCtrl.m_pWndList->DeleteAllItems();
		//m_mapExtCtrl.m_pWndList->DeleteAllItems();

		m_modelCtrl.SetWindowText(WBSF::GetFM().Model().GetLocalPath());
		m_appPathCtrl.SetWindowText(WBSF::GetFM().GetAppPath());

		std::string w = WBSF::GetFM().GetWeatherPath();
		std::string m = WBSF::GetFM().MapInput().GetGlobalPaths();
		std::string e = WBSF::GetFM().MapInput().GetExtensions();


		WBSF::StringVector W(w, "|\r\n");
		for (size_t i = 0; i < W.size(); i++)
			m_weatherPathCtrl.AddItem(CString(W[i].c_str()));

		WBSF::StringVector M(m, "|\r\n");
		for (size_t i = 0; i < M.size(); i++)
			m_mapPathCtrl.AddItem(CString(M[i].c_str()));

		WBSF::StringVector E(e, "|\r\n");
		for (size_t i = 0; i < E.size(); i++)
			m_mapExtCtrl.AddItem(CString(E[i].c_str()));


		m_projectWeatherPathCtrl.SetWindowText(WBSF::GetFM().Normals().GetLocalPath());
		m_projectMapPathCtrl.SetWindowText(WBSF::GetFM().MapInput().GetLocalPath());

		return TRUE;
	}


	void COptionDir::OnOK()
	{
		//if page is init
		if (m_appPathCtrl.GetSafeHwnd() != NULL)
		{
			CString W;
			for (int i = 0; i < m_weatherPathCtrl.GetCount(); i++)
				W += m_weatherPathCtrl.GetItemText(i) + _T("|");

			CString M;
			for (int i = 0; i < m_mapPathCtrl.GetCount(); i++)
				M += m_mapPathCtrl.GetItemText(i) + _T("|");

			CString E;
			for (int i = 0; i < m_mapExtCtrl.GetCount(); i++)
				E += m_mapExtCtrl.GetItemText(i) + _T("|");

			std::string w = CStringA(W);
			std::string m = CStringA(M);
			std::string e = CStringA(E);

			GetFM().SetWeatherPath(w);
			GetFM().MapInput().SetGlobalPaths(m);
			GetFM().MapInput().SetExtensions(e);
			GetFM().SaveDefaultFileManager();
		}

		CMFCPropertyPage::OnOK();
	}

}