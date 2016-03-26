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
#include "Basic/Callback.h"
#include "Basic/GeoBasic.h"
#include "Basic/Registry.h"
#include "geomatic/UtilGDAL.h"
#include "ModelBase/Model.h"

#include "UI/Common/NewNameDlg.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/AskDirectoryDlg.h"
#include "UI/ModelDlg.h"
#include "DBEditorPages.h"
#include "DBEditorPropSheet.h"
#include "WeatherBasedSimulationString.h"


using namespace UtilWin;
using namespace std;


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{
	//********************************************************************************
	// CNormaMPage property page

	BEGIN_MESSAGE_MAP(CNormalMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()


	CNormalMListBox::CNormalMListBox()
	{}


	CWnd* CNormalMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_LINK | AFX_VSLISTBOX_BTN_OPTION);
		OnInitList();

		return m_pWndList;
	}

	void CNormalMListBox::OnEditItem(int iItem)
	{
		std::string name = CStringA(GetItemText(iItem));
		std::string filePath = GetManager().GetFilePath(name);
		
		CallApplication(CRegistry::NORMAL_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);
	}

	void CNormalMListBox::OnLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_NORMALS);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			std::string filePath = CStringA(openDialog.GetPathName());
			std::string path = WBSF::GetPath(filePath);
			std::string title = WBSF::GetFileTitle(filePath);

			if (!WBSF::GetFM().IsInWeatherPath(path))
			{
				std::string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				OnInitList();
			}
		}
	}
	//********************************************************************************
	// CNormalMPage property page



	BEGIN_MESSAGE_MAP(CCommonMPage, CMFCPropertyPage)
		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_NORMALDB_LIST, OnSelChange)
		ON_WM_SIZE()
	END_MESSAGE_MAP()


	CCommonMPage::CCommonMPage(UINT nIDTemplate, UINT nIDCaption ) :
		CMFCPropertyPage(nIDTemplate, nIDCaption),
		m_bInit(false)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CCommonMPage::~CCommonMPage()
	{}

	

	void CCommonMPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);

		if (!m_pList)
		{
			m_pList = GetList();
			ASSERT(m_pList);
		}

		DDX_Control(pDX, IDC_NORMALDB_LIST, *m_pList);
		DDX_Control(pDX, IDC_NORMALDB_FILEPATH, m_filePathCtrl);
		m_pList->SetFocus();
	}

	void CCommonMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	{
		pResult = 0;

		std::string filePath;
		int iItem = m_pList->GetSelItem();
		if (iItem != -1)
		{
			std::string fileName = CStringA(m_pList->GetItemText(iItem));
			filePath = GetManager().GetFilePath(fileName);
		}
			
		//else
			//filePath = GetManager().GetDefa

		m_filePathCtrl.SetWindowText(CString(filePath.c_str()));
	}

	void CCommonMPage::OnSize(UINT nType, int cx, int cy)
	{
		if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
			return;
	
		CRect rect;
		m_filePathCtrl.GetClientRect(rect);
		
		m_pList->SetWindowPos(NULL, 5, 5, cx - 2 * 5 - 100, cy - rect.Height() - 3 * 5, SWP_NOZORDER | SWP_NOACTIVATE);
		m_filePathCtrl.SetWindowPos(NULL, 5, cy - rect.Height() - 5, cx - 2 * 5 - 100, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}

	//********************************************************************************
	// CNormalMPage property page

	BEGIN_MESSAGE_MAP(CNormalMPage, CCommonMPage)
	END_MESSAGE_MAP()


	CNormalMPage::CNormalMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_NORMAL_PAGE)
	{
	}


	CBioSIMListBoxPtr CNormalMPage::GetList(){ return std::make_shared<CNormalMListBox>(); }
	const CDirectoryManager& CNormalMPage::GetManager()const { return WBSF::GetFM().Normals(); }

	//********************************************************************************
	// CDailyMPage property page

	BEGIN_MESSAGE_MAP(CDailyMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CDailyMListBox::CDailyMListBox()
	{}
	 

	CWnd* CDailyMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;
		 
		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_LINK | AFX_VSLISTBOX_BTN_OPTION);
		OnInitList();

		return m_pWndList;
	}


	void CDailyMListBox::CreateNewItem()
	{
		CAskDirectoryDlg askDirDlg;
		askDirDlg.m_directoryArray.Tokenize(GetManager().GetDirectories(), "|");

		if (askDirDlg.DoModal() == IDOK)
		{
			CNewNameDlg newNameDlg;
			while (newNameDlg.DoModal() == IDOK)
			{
				std::string path = askDirDlg.m_directoryArray[askDirDlg.m_selectedDir];
				std::string filePath = path + newNameDlg.m_name + GetManager().GetExtensions().c_str();
				ERMsg msg;
				msg = GetManager().CreateNewDataBase(filePath);
				if (msg)
				{
					OnInitList();
					break;//exit DoModal
				}
				else
				{
					SYShowMessage(msg, this);
				}
			}
		}
	}

	void CDailyMListBox::OnEditItem(int iItem)
	{
		std::string name = CStringA(GetItemText(iItem));
		std::string filePath = GetManager().GetFilePath(name);

		CallApplication(CRegistry::DAILY_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);
	}

	void CDailyMListBox::OnLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_DAILY);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			std::string filePath = CStringA(openDialog.GetPathName());
			std::string path = WBSF::GetPath(filePath);

			if (!WBSF::GetFM().IsInWeatherPath(path))
			{
				std::string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();
				OnInitList();
			}
		}
	}

	//********************************************************************************
	// CDailyFMsPage property page



	BEGIN_MESSAGE_MAP(CDailyMPage, CCommonMPage)
		//ON_NOTIFY(ON_BLB_SELCHANGE, IDC_DAILYDB_LIST, OnSelChange)
		//ON_WM_SIZE()
	END_MESSAGE_MAP()

	CDailyMPage::CDailyMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_DAILY_PAGE)
		//CCommonMPage(IDD_NORMAL_PAGE),
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CDailyMPage::~CDailyMPage()
	//{}


	void CDailyMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DAILYDB_LIST, m_list);
		//DDX_Control(pDX, IDC_DAILYDB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();
	}


	CBioSIMListBoxPtr CDailyMPage::GetList(){ return std::make_shared<CDailyMListBox>(); }
	const CDirectoryManager& CDailyMPage::GetManager()const { return WBSF::GetFM().Daily(); }



	//void CDailyMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().Daily().GetFilePath(filePath);
	//	}

	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}

	//void CDailyMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}
	//	
	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}
	//********************************************************************************
	// CHourlyMListBox property page

	BEGIN_MESSAGE_MAP(CHourlyMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CHourlyMListBox::CHourlyMListBox()
	{}


	CWnd* CHourlyMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_LINK | AFX_VSLISTBOX_BTN_OPTION);
		OnInitList();

		return m_pWndList;
	}


	void CHourlyMListBox::CreateNewItem()
	{
		CAskDirectoryDlg askDirDlg;
		askDirDlg.m_directoryArray.Tokenize(GetManager().GetDirectories(), "|");

		if (askDirDlg.DoModal() == IDOK)
		{
			CNewNameDlg newNameDlg;
			while (newNameDlg.DoModal() == IDOK)
			{
				std::string path = askDirDlg.m_directoryArray[askDirDlg.m_selectedDir];
				std::string filePath = path + newNameDlg.m_name + GetManager().GetExtensions().c_str();
				ERMsg msg;
				msg = GetManager().CreateNewDataBase(filePath);
				if (msg)
				{
					OnInitList();
					break;//exit DoModal
				}
				else
				{
					SYShowMessage(msg, this);
				}
			}
		}
	}

	void CHourlyMListBox::OnEditItem(int iItem)
	{
		std::string name = CStringA(GetItemText(iItem));
		std::string filePath = GetManager().GetFilePath(name);

		CallApplication(CRegistry::HOURLY_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);
	}

	void CHourlyMListBox::OnLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_HOURLY);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			std::string filePath = CStringA(openDialog.GetPathName());
			std::string path = WBSF::GetPath(filePath);

			if (!WBSF::GetFM().IsInWeatherPath(path))
			{
				std::string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				OnInitList();
			}
		}
	}

	//********************************************************************************
	// CHourlyMPage property page

	BEGIN_MESSAGE_MAP(CHourlyMPage, CCommonMPage)
		////ON_NOTIFY(ON_BLB_SELCHANGE, IDC_DB_LIST, OnSelChange)
		//ON_WM_SIZE()
	END_MESSAGE_MAP()

	CHourlyMPage::CHourlyMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_HOURLY_PAGE)
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CHourlyMPage::~CHourlyMPage()
	//{}

	void CHourlyMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();
	}

	CBioSIMListBoxPtr CHourlyMPage::GetList(){ return std::make_shared<CHourlyMListBox>(); }
	const CDirectoryManager& CHourlyMPage::GetManager()const { return WBSF::GetFM().Hourly(); }





	//void CHourlyMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().Hourly().GetFilePath(filePath);
	//	}

	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}

	//void CHourlyMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}

	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}
	//********************************************************************************
	// CGribMListBox property page

	BEGIN_MESSAGE_MAP(CGribMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CGribMListBox::CGribMListBox()
	{}


	CWnd* CGribMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_LINK | AFX_VSLISTBOX_BTN_OPTION);
		OnInitList();

		return m_pWndList;
	}


	void CGribMListBox::CreateNewItem()
	{
		CAskDirectoryDlg askDirDlg;
		askDirDlg.m_directoryArray.Tokenize(GetManager().GetDirectories(), "|");

		if (askDirDlg.DoModal() == IDOK)
		{
			CNewNameDlg newNameDlg;
			while (newNameDlg.DoModal() == IDOK)
			{
				std::string path = askDirDlg.m_directoryArray[askDirDlg.m_selectedDir];
				std::string filePath = path + newNameDlg.m_name + GetManager().GetExtensions().c_str();
				ERMsg msg;
				msg = GetManager().CreateNewDataBase(filePath);
				if (msg)
				{
					OnInitList();
					break;//exit DoModal
				}
				else
				{
					SYShowMessage(msg, this);
				}
			}
		}
	}

	void CGribMListBox::OnEditItem(int iItem)
	{
	}

	void CGribMListBox::OnLink()
	{

		CString filter = UtilWin::GetCString(IDS_STR_FILTER_GRIBS);
		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			std::string filePath = CStringA(openDialog.GetPathName());
			std::string path = WBSF::GetPath(filePath);

			if (!WBSF::GetFM().IsInWeatherPath(path))
			{
				std::string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				OnInitList();
			}
		}
	}

	//********************************************************************************
	// CGribMPage property page
	BEGIN_MESSAGE_MAP(CGribMPage, CCommonMPage)
		//ON_NOTIFY(ON_BLB_SELCHANGE, IDC_DB_LIST, OnSelChange)
		//ON_WM_SIZE()
	END_MESSAGE_MAP()


	CGribMPage::CGribMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_GRIB_PAGE)
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CGribMPage::~CGribMPage()
	//{}

	void CGribMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();
	}

	CBioSIMListBoxPtr CGribMPage::GetList(){ return std::make_shared<CGribMListBox>(); }
	const CDirectoryManager& CGribMPage::GetManager()const { return WBSF::GetFM().Gribs(); }




	//void CGribMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().Grib().GetFilePath(filePath);
	//	}

	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}

	//void CGribMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}

	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}
	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CMapInputMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CMapInputMListBox::CMapInputMListBox() :CBioSIMListBox(false, WBSF::FILE_NAME)
	{}


	CWnd* CMapInputMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_SHOW_INFO | AFX_VSLISTBOX_BTN_SHOWMAP | AFX_VSLISTBOX_BTN_LINK | AFX_VSLISTBOX_BTN_OPTION);
		OnInitList();

		return m_pWndList;
	}

	void CMapInputMListBox::OnEditItem(int iItem)
	{
		OnShowMap(iItem);
	}

	void CMapInputMListBox::OnShowInfo(int iItem)
	{
		if (iItem != -1)
		{
			std::string mapName = CStringA(GetItemText(iItem));
			std::string filePath = WBSF::GetTempPath() + mapName + ".txt";

			ofStream file;
			ERMsg msg = file.open(filePath);
			if (msg)
			{
				filePath = WBSF::GetFM().MapInput().GetFilePath(mapName);
				ASSERT(!filePath.empty());

				CNewGeoFileInfo info;
				msg = GetGDALInfo(filePath, info);
				if (msg)
				{
					file << info.m_filePath << endl << endl;
					file << info.m_description << endl << endl;
					file << info.m_fileType << endl << endl;
					file << info.m_projection << endl << endl;
					file << info.m_info << endl << endl;
					file << info.m_zUnits << endl << endl;
					file << info.m_noData << endl << endl;
				}
				else
				{

					file << SYGetText(msg);
				}

				file.close();
			}


			CallApplication(CRegistry::TEXT_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);
		}
	}

	void CMapInputMListBox::OnLink()
	{
		WBSF::CRegistry registry("Last open path");
		//CString  folder = Convert(registry.GetProfileString("MAP"));
		
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_LAYER);
		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);
		//openDialog.m_ofn.lpstrInitialDir = folder;
		openDialog.m_ofn.nFilterIndex = registry.GetValue<int>("Import map filter index", 0);


		if (openDialog.DoModal() == IDOK)
		{
			registry.SetValue("Import map filter index", openDialog.m_ofn.nFilterIndex);

			ERMsg msg;
			CString sOutMessage;



			std::string filePath;
			POSITION pos = openDialog.GetStartPosition();
			while (pos != NULL)
			{
				filePath = CStringA(openDialog.GetNextPathName(pos));
				std::string path = WBSF::GetPath(filePath);
				std::string ext = WBSF::GetFileExtension(filePath);
				if (WBSF::IsEqualNoCase(WBSF::Right(filePath, 8), ".aux.xml"))
					ext = WBSF::Right(filePath, 8);

				if (!WBSF::GetFM().MapInput().IsInDirectoryList(path))
					WBSF::GetFM().MapInput().AppendToDirectoryList(path);

				if (!WBSF::GetFM().MapInput().IsInExtensionList(ext))
					WBSF::GetFM().MapInput().AppendToExtensionList(ext);
			}

			WBSF::GetFM().SaveDefaultFileManager();
			OnInitList();

			registry.SetValue("MAP", WBSF::GetPath(filePath));
		}
	}

	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CMapInputMPage, CCommonMPage)
		//		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_DB_LIST, OnSelChange)
		//	ON_WM_SIZE()
	END_MESSAGE_MAP()

	CMapInputMPage::CMapInputMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_MAPINPUT_PAGE)
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CMapInputMPage::~CMapInputMPage()
	//{}
	void CMapInputMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();

		if (!pDX->m_bSaveAndValidate)
			m_pList->SelectString(m_lastDEMName);
	}


	CBioSIMListBoxPtr CMapInputMPage::GetList(){ return std::make_shared<CMapInputMListBox>(); }
	const CDirectoryManager& CMapInputMPage::GetManager()const { return WBSF::GetFM().MapInput(); }




	//void CMapInputMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().MapInput().GetFilePath(filePath);
	//		m_lastDEMName = UtilWin::GetFileName(CString(filePath.c_str()));
	//	}


	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}
	//void CMapInputMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}

	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}
	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CModelMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CModelMListBox::CModelMListBox()
	{}


	CWnd* CModelMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT);
		OnInitList();

		return m_pWndList;
	}


	void CModelMListBox::OnAfterAddItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		ERMsg msg = WBSF::GetFM().Model().Set(name, CModel());
		if (!msg)
		{
			SYShowMessage(msg, this);
			RemoveItem(iItem);
		}
	}

	void CModelMListBox::OnEditItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		/*
		string filePath = WBSF::GetFM().WeatherUpdate().GetFilePath(name);
		CallApplication(CRegistry::MODEL_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);
*/
		CModelDlg dlg(this); 
		ENSURE(WBSF::GetFM().Model().Get(name, dlg.m_model));
		ASSERT(dlg.m_model.GetName() == name);

		if (dlg.DoModal() == IDOK)
		{
			ERMsg msg = GetFM().Model().Set(name, dlg.m_model);
			if (!msg)
				SYShowMessage(msg, this);
		}
	}


	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CModelMPage, CCommonMPage)
	END_MESSAGE_MAP()


	CModelMPage::CModelMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_MODEL_PAGE)
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CModelMPage::~CModelMPage()
	//{}

	void CModelMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();

	}


	CBioSIMListBoxPtr CModelMPage::GetList(){ return std::make_shared<CModelMListBox>(); }
	const CDirectoryManager& CModelMPage::GetManager()const { return WBSF::GetFM().Model(); }



	//void CModelMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().Model().GetFilePath(filePath);
	//	}
	//	else
	//	{
	//		filePath = WBSF::GetFM().Model().GetLocalPath();
	//	}

	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}
	//void CModelMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}

	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}

	//********************************************************************************
	// CWeatherUpdatePage property page

	BEGIN_MESSAGE_MAP(CWeatherUpdateMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CWeatherUpdateMListBox::CWeatherUpdateMListBox()
	{}


	CWnd* CWeatherUpdateMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT);
		OnInitList();

		return m_pWndList;
	}


	void CWeatherUpdateMListBox::OnAfterAddItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		WBSF::GetFM().WeatherUpdate().GetFilePath(name);
		zen::XmlDoc doc;
		ERMsg msg = zen::SaveXML(WBSF::GetFM().WeatherUpdate().GetLocalPath() + name + WBSF::GetFM().WeatherUpdate().GetExtensions(), "", "", "");

		if (!msg)
		{
			SYShowMessage(msg, this);
			RemoveItem(iItem);
		}
	}

	void CWeatherUpdateMListBox::OnEditItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		string filePath = WBSF::GetFM().WeatherUpdate().GetFilePath(name);

		if (!filePath.empty())
			CallApplication(CRegistry::WEATHER_UPDATER, filePath, GetSafeHwnd(), SW_SHOW);

	}


	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CWeatherUpdateMPage, CCommonMPage)
	END_MESSAGE_MAP()

	CWeatherUpdateMPage::CWeatherUpdateMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_WEATHERUPDATE_PAGE)
	{}

	void CWeatherUpdateMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		//DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();
	}

	CBioSIMListBoxPtr CWeatherUpdateMPage::GetList(){ return std::make_shared<CWeatherUpdateMListBox>(); }
	const CDirectoryManager& CWeatherUpdateMPage::GetManager()const { return WBSF::GetFM().WeatherUpdate(); }




	//void CWeatherUpdateMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	//{
	//	pResult = 0;

	//	std::string filePath;
	//	int iItem = m_list.GetSelItem();
	//	if (iItem != -1)
	//	{
	//		filePath = CStringA(m_list.GetItemText(iItem));
	//		filePath = WBSF::GetFM().WeatherUpdate().GetFilePath(filePath);
	//	}
	//	else
	//	{
	//		filePath = WBSF::GetFM().WeatherUpdate().GetLocalPath();
	//	}

	//	m_filePathCtrl.SetWindowText(Convert(filePath));
	//}

	//void CWeatherUpdateMPage::OnSize(UINT nType, int cx, int cy)
	//{
	//	if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
	//		return;

	//	if (!m_bInit)
	//	{
	//		GetClientRect(m_rect);
	//		m_bInit = true;
	//	}

	//	CRect rect;
	//	GetClientRect(rect);
	//	int dx = rect.Width() - m_rect.Width();
	//	int dy = rect.Height() - m_rect.Height();
	//	m_rect = rect;

	//	//CRect rect;
	//	m_list.GetWindowRect(rect); ScreenToClient(rect);
	//	m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	//	m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
	//	m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	//}
	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CScriptMListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CScriptMListBox::CScriptMListBox()
	{}


	CWnd* CScriptMListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();
		pWnd->ModifyStyle(NULL, LVS_SORTASCENDING);

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT);
		OnInitList();

		return m_pWndList;
	}


	void CScriptMListBox::OnAfterAddItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		string filePath = WBSF::GetFM().Script().GetLocalPath() + name + WBSF::GetFM().Script().GetExtensions();
		ofStream file;
		ERMsg msg = file.open(filePath);

		if (!msg)
		{
			SYShowMessage(msg, this);
			RemoveItem(iItem);
		}

	}

	void CScriptMListBox::OnEditItem(int iItem)
	{
		string name = CStringA(GetItemText(iItem));
		string filePath = WBSF::GetFM().Script().GetFilePath(name);

		if (!filePath.empty())
			CallApplication(CRegistry::TEXT_EDITOR, filePath, GetSafeHwnd(), SW_SHOW);

	}


	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CScriptMPage, CCommonMPage)
		//ON_NOTIFY(ON_BLB_SELCHANGE, IDC_DB_LIST, OnSelChange)
		//ON_WM_SIZE()
	END_MESSAGE_MAP()


	CScriptMPage::CScriptMPage() :
		CCommonMPage(IDD_NORMAL_PAGE, IDS_CMN_SCRIPT_PAGE)
		//m_bInit(false)
	{
		//m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	//CScriptMPage::~CScriptMPage()
	//{}

	void CScriptMPage::DoDataExchange(CDataExchange* pDX)
	{
		CCommonMPage::DoDataExchange(pDX);

		////DDX_Control(pDX, IDC_DB_LIST, m_list);
		//DDX_Control(pDX, IDC_DB_FILEPATH, m_filePathCtrl);
		//m_list.SetFocus();
	}

CBioSIMListBoxPtr CScriptMPage::GetList(){ return std::make_shared<CScriptMListBox>(); }
const CDirectoryManager& CScriptMPage::GetManager()const { return WBSF::GetFM().Script(); }

	
//
//	void CScriptMPage::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
//	{
//		pResult = 0;
//
//		std::string filePath;
//		int iItem = m_list.GetSelItem();
//		if (iItem != -1)
//		{
//			filePath = CStringA(m_list.GetItemText(iItem));
//			filePath = WBSF::GetFM().Script().GetFilePath(filePath);
//		}
//		else
//		{
//			filePath = WBSF::GetFM().Script().GetLocalPath();
//		}
//
//		m_filePathCtrl.SetWindowText(Convert(filePath));
//	}
//
//	void CScriptMPage::OnSize(UINT nType, int cx, int cy)
//	{
//		if (GetSafeHwnd() == NULL || m_filePathCtrl.GetSafeHwnd() == NULL)
//			return;
//
//		if (!m_bInit)
//		{
//			GetClientRect(m_rect);
//			m_bInit = true;
//		}
//
//		CRect rect;
//		GetClientRect(rect);
//		int dx = rect.Width() - m_rect.Width();
//		int dy = rect.Height() - m_rect.Height();
//		m_rect = rect;
//
//		//CRect rect;
//		m_list.GetWindowRect(rect); ScreenToClient(rect);
//		m_list.SetWindowPos(NULL, 0, 0, rect.Width() + dx, rect.Height() + dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
//
//		m_filePathCtrl.GetWindowRect(rect); ScreenToClient(rect);
//		m_filePathCtrl.SetWindowPos(NULL, rect.left, rect.top + dy, rect.Width() + dx, rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
//	}
}