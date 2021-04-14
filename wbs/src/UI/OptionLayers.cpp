//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2021	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "OptionLayers.h"
#include "UI/Common/UtilWin.h"
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
	BEGIN_MESSAGE_MAP(CLayersPropertyGridCtrl, CMFCPropertyGridCtrl)
		ON_WM_DESTROY()
	END_MESSAGE_MAP()

	CLayersPropertyGridCtrl::CLayersPropertyGridCtrl()
	{
	}
	
	void CLayersPropertyGridCtrl::Init()
	{
		CAppOption options;

		CRect rect;
		GetClientRect(rect);
		m_nLeftColumnWidth = options.GetProfileInt(_T("LayersLinkPropertiesSplitterPos"), rect.Width() / 2);
		
		CMFCPropertyGridCtrl::Init();
	}

	void CLayersPropertyGridCtrl::Initialize()
	{
		RemoveAll();

		
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_LINK_LAYERS_HEADER));
		
		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
		SetVSDotNetLook(true);

		//General
		StringVector name(IDS_STR_LINK_LAYERS, "|;");
		ASSERT(name.size() == NB_GEO_LAYERS_KEY);

		string filter = GetString(IDS_STR_FILTER_LAYER);

		CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(_T("Layers"), -1);
		for (size_t i = 0; i < NB_GEO_LAYERS_KEY; i++)
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

	void CLayersPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		//CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		CLayersPropertyGridCtrl& me = const_cast<CLayersPropertyGridCtrl&>(*this);

		int i = (int)pPropIn->GetData();
		me.m_filePath[i] = CStringA(pPropIn->GetValue());
	}


	void CLayersPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CLayersPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CLayersPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
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

	void CLayersPropertyGridCtrl::OnDestroy()
	{
		CAppOption options;
		options.WriteProfileInt(_T("LayersLinkPropertiesSplitterPos"), m_nLeftColumnWidth);


		CMFCPropertyGridCtrl::OnDestroy();
	}


	/////////////////////////////////////////////////////////////////////////////
	// COptionLayerPage property page

	COptionLayerPage::COptionLayerPage() : CMFCPropertyPage(COptionLayerPage::IDD)
	{
	}



	COptionLayerPage::~COptionLayerPage()
	{
	}

	void COptionLayerPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_PROPERTIES, m_ctrl);


	}


	BEGIN_MESSAGE_MAP(COptionLayerPage, CMFCPropertyPage)
	END_MESSAGE_MAP()

	
	//static const char* DEFAULT_APP_TITLE[CLayersPropertyGridCtrl::NB_LAYERS] =
	//{
	//	"DEM (lo)", "DEM (hi)", "Administrative (lo)", "Administrative (hi)",
	//};

	BOOL COptionLayerPage::OnInitDialog()
	{
		CMFCPropertyPage::OnInitDialog();

		CRegistry registry;
		for (size_t i = 0; i < NB_GEO_LAYERS_KEY; i++)
		{
			m_ctrl.m_filePath[i] = registry.GetString(CRegistry::GetGeoRegistryKey(i));
		}


		m_ctrl.Initialize();

		return TRUE;
	}



	void COptionLayerPage::OnOK()
	{
		if (m_ctrl.GetSafeHwnd())
		{
			CRegistry registry;
			for (size_t i = 0; i < NB_GEO_LAYERS_KEY; i++)
				registry.SetString(CRegistry::GetGeoRegistryKey(i), m_ctrl.m_filePath[i]);

		}


		CMFCPropertyPage::OnOK();
	}

}