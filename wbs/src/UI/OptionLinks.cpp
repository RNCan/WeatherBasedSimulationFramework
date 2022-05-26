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
#include "OptionLinks.h"
#include "UI/Common/UtilWin.h"
#include "Basic/Registry.h"
#include "WeatherBasedSimulationString.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h"


using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




namespace WBSF
{
	BEGIN_MESSAGE_MAP(CAppLinkPropertyGridCtrl, CMFCPropertyGridCtrl)
		ON_WM_DESTROY()
	END_MESSAGE_MAP()

	CAppLinkPropertyGridCtrl::CAppLinkPropertyGridCtrl()
	{
	}
	
	void CAppLinkPropertyGridCtrl::Init()
	{
		CAppOption options;

		CRect rect;
		GetClientRect(rect);
		m_nLeftColumnWidth = options.GetProfileInt(_T("AppLinkPropertiesSplitterPos"), rect.Width() / 2);
		
		CMFCPropertyGridCtrl::Init();
	}

	void CAppLinkPropertyGridCtrl::Initialize()
	{
		RemoveAll();

		
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_APP_LINK_HEADER));
		
		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
		SetVSDotNetLook(true);

		//General
		StringVector name(IDS_STR_APP_LINK, "|;");
		ASSERT(name.size() == NB_LINKS);

		string filter = GetString(IDS_STR_FILTER_EXECUTABLE);

		CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(_T("Links"), -1);
		for (size_t i = 1; i < NB_LINKS; i++)
		{
			CString n = CString(name[i].c_str());
			CString v = CString(m_filePath[i].c_str());
			CString f = CString(filter.c_str());
			CMFCPropertyGridFileProperty* pIten = new CMFCPropertyGridFileProperty(n, TRUE, v, NULL, NULL, f, NULL, i);
			pGeneral->AddSubItem(pIten);
		}


		AddProperty(pGeneral);

		EnableWindow(true);
		EnableProperties(true);


	}

	void CAppLinkPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		//CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		CAppLinkPropertyGridCtrl& me = const_cast<CAppLinkPropertyGridCtrl&>(*this);

		int i = (int)pPropIn->GetData();
		me.m_filePath[i] = CStringA(pPropIn->GetValue());
	}


	void CAppLinkPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CAppLinkPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CAppLinkPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
	{

		if (pMsg->message == WM_KEYDOWN)
		{
			BOOL bAlt = GetKeyState(VK_CONTROL) & 0x8000;
			if (pMsg->wParam == VK_RETURN)
			{
				if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
				{
					if (m_pSel->GetOptionCount()>0)
						OnSelectCombo();

					EndEditItem();

					//select next item
					size_t ID = m_pSel->GetData();
					size_t newID = (ID + 1) % GetProperty(0)->GetSubItemsCount();
					CMFCPropertyGridProperty* pNext = FindItemByData(newID);
					if (pNext)
					{
						SetCurSel(pNext);
						EditItem(pNext);
					}

					return TRUE; // this doesn't need processing anymore
				}

			}
			else if (pMsg->wParam == VK_DOWN && bAlt)
			{
				m_pSel->OnClickButton(CPoint(-1, -1));
				return TRUE; // this doesn't need processing anymore
			}
		}

		return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
	}

	void CAppLinkPropertyGridCtrl::OnDestroy()
	{
		CAppOption options;
		options.WriteProfileInt(_T("AppLinkPropertiesSplitterPos"), m_nLeftColumnWidth);


		CMFCPropertyGridCtrl::OnDestroy();
	}


	/////////////////////////////////////////////////////////////////////////////
	// COptionLinks property page

	COptionLinks::COptionLinks() : CMFCPropertyPage(COptionLinks::IDD)
	{
	}



	COptionLinks::~COptionLinks()
	{
	}

	void COptionLinks::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_PROPERTIES, m_ctrl);


	}


	BEGIN_MESSAGE_MAP(COptionLinks, CMFCPropertyPage)
	END_MESSAGE_MAP()

	static const char* REGISTRY_NAME[CAppLinkPropertyGridCtrl::NB_LINKS] =
	{
		CRegistry::BIOSIM, CRegistry::SHOWMAP, CRegistry::HOURLY_EDITOR, CRegistry::DAILY_EDITOR, CRegistry::NORMAL_EDITOR, CRegistry::MODEL_EDITOR,
		CRegistry::MATCH_STATION, CRegistry::WEATHER_UPDATER, CRegistry::FTP_TRANSFER, CRegistry::TDATE, CRegistry::MERGEFILE,
		CRegistry::TEXT_EDITOR, CRegistry::XML_EDITOR, CRegistry::SPREADSHEET1, CRegistry::SPREADSHEET2, CRegistry::GIS, CRegistry::R_SCRIPT,
	};

	static const char* DEFAULT_APP_TITLE[CAppLinkPropertyGridCtrl::NB_LINKS] =
	{
		"BioSIM11", "ShowMap", "HourlyEditor", "DailyEditor", "NormalsEditor", "ModelEditor",
		"MatchStation", "WeatherUpdater", "WinSCP", "TDate", "MergeFiles",
		"Notepad", "NotePad", "Excel", "Calc", "QGIS", "Rscript",
	};

	BOOL COptionLinks::OnInitDialog()
	{
		CMFCPropertyPage::OnInitDialog();

		CRegistry registry;
		for (size_t i = 0; i < CAppLinkPropertyGridCtrl::NB_LINKS; i++)
		{
			m_ctrl.m_filePath[i] = registry.GetAppFilePath(REGISTRY_NAME[i]);
			if (m_ctrl.m_filePath[i].empty())
				m_ctrl.m_filePath[i] = string(DEFAULT_APP_TITLE[i]) + ".exe";
		}


		m_ctrl.Initialize();

		return TRUE;
	}



	void COptionLinks::OnOK()
	{
		if (m_ctrl.GetSafeHwnd())
		{
			CRegistry registry;
			for (size_t i = 0; i < CAppLinkPropertyGridCtrl::NB_LINKS; i++)
				registry.SetAppFilePath(REGISTRY_NAME[i], m_ctrl.m_filePath[i]);

		}


		CMFCPropertyPage::OnOK();
	}

}