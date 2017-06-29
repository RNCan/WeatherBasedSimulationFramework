//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 10/12/2014	Rémi Saint-Amant	Creation from old interface
//****************************************************************************

#include "stdafx.h"


#include "FileManager/FileManager.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/ParametersVariationsDlg.h"
#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWGInputDlg dialog
using namespace UtilWin;
using namespace std;


namespace WBSF
{
	//*****************************************************************************************************

	enum TParametersVariationsProperties{ PV_MIN, PV_MAX, PV_STEP, NB_PROPERTIES };

	BEGIN_MESSAGE_MAP(CParametersVariationsProperties, CMFCPropertyGridCtrl)
		ON_WM_ENABLE()
	END_MESSAGE_MAP()

	CParametersVariationsProperties::CParametersVariationsProperties(const CModelInputParameterDefVector& parametersDefinition, CParametersVariationsDefinition& parametersVariations) :
		m_parametersDefinition(parametersDefinition),
		m_parametersVariations(parametersVariations)
	{
		m_curP = UNKNOWN_POS;
	}

	void CParametersVariationsProperties::Init()
	{
		CMFCPropertyGridCtrl::Init();

		RemoveAll();

		CRect rect;
		GetClientRect(rect);

		//add all attribute
		CAppOption options;
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
		m_nLeftColumnWidth = options.GetProfileInt(_T("ParametersVariationsSplitterPos"), rect.Width() / 2);
		SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));

		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
		EnableDescriptionArea(false);
		SetVSDotNetLook(true);
		MarkModifiedProperties(true);
		SetAlphabeticMode(false);
		SetShowDragContext(true);
		EnableWindow(true);
		AlwaysShowUserToolTip();

		//General
		//CStringArrayEx section(IDS_SIM_DISPERSAL_SECTION);
		WBSF::StringVector name(IDS_SIM_PARAMETERS_VARIATIONS_PROPERTIES, "|");
		WBSF::StringVector description(IDS_SIM_PARAMETERS_VARIATIONS_DESCRIPTION, "|");



		//CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(propertyHeader[0], -1);
		AddProperty(new CStdGridProperty(name[PV_MIN], 0, description[PV_MIN], PV_MIN));
		AddProperty(new CStdGridProperty(name[PV_MAX], 0, description[PV_MAX], PV_MAX));
		AddProperty(new CStdGridProperty(name[PV_STEP], 0, description[PV_STEP], PV_STEP));
		SetSelection(0);
	}


	void CParametersVariationsProperties::SetSelection(size_t p)
	{
		ASSERT(m_parametersVariations.size() == m_parametersDefinition.size());

		//if (p != m_curP)
		{
			

			
			for (int i = 0; i < NB_PROPERTIES; i++)
			{
				string str;
				if (p < m_parametersVariations.size())
				{
					switch (i)
					{
					case PV_MIN:	str = ToString(m_parametersVariations[p].m_min); break;
					case PV_MAX:	str = ToString(m_parametersVariations[p].m_max); break;
					case PV_STEP:	str = ToString(m_parametersVariations[p].m_step); break;
					default: ASSERT(false);
					}
				}

				CStdGridProperty* pProp = static_cast<CStdGridProperty*>(FindItemByData(i));
				pProp->set_string(str);

				ASSERT(pProp);

			}

			if (p != m_curP)
				EnableProperties(p < m_parametersVariations.size());

			m_curP = p;

		}
	}

	void CParametersVariationsProperties::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		ASSERT(m_parametersVariations.size() == m_parametersDefinition.size());

		CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		CParametersVariationsProperties& me = const_cast<CParametersVariationsProperties&>(*this);
		std::string str = pProp->get_string();


		int i = (int)pProp->GetData();
		switch (i)
		{
		case PV_MIN:	me.m_parametersVariations[m_curP].m_min =  ToFloat(str); break;
		case PV_MAX:	me.m_parametersVariations[m_curP].m_max =  ToFloat(str); break;
		case PV_STEP:	me.m_parametersVariations[m_curP].m_step = ToFloat(str); break;
		default: ASSERT(false);
		}
	}



	void CParametersVariationsProperties::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CParametersVariationsProperties::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CParametersVariationsProperties::PreTranslateMessage(MSG* pMsg)
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

	//
	BOOL CParametersVariationsProperties::ValidateItemData(CMFCPropertyGridProperty* pPropIn)
	{
		BOOL bValid = TRUE;

		CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		const CParametersVariationsProperties& me = const_cast<CParametersVariationsProperties&>(*this);

		int i = (int)pProp->GetData();

		if (i != PV_STEP)
		{
			std::string str = pProp->get_string();
			double val = WBSF::ToDouble(str);
			double Vmin = WBSF::ToDouble(m_parametersDefinition[m_curP].m_min);
			double Vmax = WBSF::ToDouble(m_parametersDefinition[m_curP].m_max);
			bValid = val >= Vmin && val <= Vmax;
		}

		return bValid;
	}

	void CParametersVariationsProperties::OnEnable(BOOL bEnable)
	{
		EnableProperties(bEnable);
	}


//***************************************************************************************************************************************

	BEGIN_MESSAGE_MAP(CParametersVariationsDlg, CDialog)
		ON_CBN_SELCHANGE(IDC_PV_GENERATION_TYPE, OnGeneratioTypeChange)
		ON_EN_CHANGE(IDC_PV_NB_VARIATIONS, OnNbVariationChange)
		ON_NOTIFY(TVN_SELCHANGED, IDC_PV_PARAMETERS, OnSelectionChange)
		ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
		ON_WM_DESTROY()
		ON_WM_ENABLE()
	END_MESSAGE_MAP()

	
	
	CParametersVariationsDlg::CParametersVariationsDlg(CWnd* pParent /*=NULL*/, bool bShowVariationType)
		: CDialog(CParametersVariationsDlg::IDD, pParent),
		m_propertiesCtrl(m_parametersDefinition, m_parametersVariations)
	{
		m_bShowVariationType = bShowVariationType;
	}


	void CParametersVariationsDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_PV_PARAMETERS, m_parametersCtrl);
		DDX_Control(pDX, IDC_PV_PROPERTIES, m_propertiesCtrl);
		DDX_Control(pDX, IDC_PV_GENERATION_TYPE, m_generationTypeCtrl);
		DDX_Control(pDX, IDC_PV_NB_VARIATIONS, m_nbVariationsCtrl);


		if (pDX->m_bSaveAndValidate)
		{
			m_parametersVariations.m_variationType = m_generationTypeCtrl.GetCurSel();
			m_parametersVariations.m_nbVariation = ToInt( m_nbVariationsCtrl.GetString());
		}
		else
		{
			m_generationTypeCtrl.SetCurSel((int)m_parametersVariations.m_variationType);
			m_nbVariationsCtrl.SetString(WBSF::ToString(m_parametersVariations.m_nbVariation));
			
			//Fill parameters
			FillParameters();
			//select parameters
			SelectParameters();
		}

	}


	


	BOOL CParametersVariationsDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		UpdateCtrl();

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = option.GetProfilePoint(_T("ParametersVariationsDlg"), CPoint(480, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		return TRUE;
	}



	void CParametersVariationsDlg::FillParameters()
	{
		string str;
		for (size_t i = 0; i < m_parametersDefinition.size(); i++)
			str += m_parametersDefinition[i].m_name + "|";


		m_parametersCtrl.SetPossibleValues(str);
	}

	void CParametersVariationsDlg::SelectParameters()
	{
		ASSERT(m_parametersVariations.size() == m_parametersDefinition.size());

		string str;
		for (size_t i = 0; i < m_parametersVariations.size(); i++)
			str.insert(str.begin(), m_parametersVariations[i].m_bActive ? '1' : '0');


		m_parametersCtrl.SetSelection(str);

		//update property
		m_propertiesCtrl.SetSelection(m_propertiesCtrl.GetSelection());
	}

	void CParametersVariationsDlg::OnSelectionChange(NMHDR* pNMHDR, LRESULT* pResult)
	{
		ASSERT(m_parametersVariations.size() == m_parametersDefinition.size());

		NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		if (hItem)
		{
			CStringA name(m_parametersCtrl.GetItemText(hItem));
			size_t pos = m_parametersVariations.Find((LPCSTR)name);
			m_propertiesCtrl.SetSelection(pos);
		}

	}


	LRESULT CParametersVariationsDlg::OnCheckbox(WPARAM wParam, LPARAM lParam)
	{
		ASSERT(m_parametersVariations.size() == m_parametersDefinition.size());

		XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pData);

		if (pData->nCtrlId == IDC_PV_PARAMETERS)
		{
			BOOL bChecked = (BOOL)lParam;

			if (pData)
			{
				HTREEITEM hItem = pData->hItem;

				if (hItem)
				{
					CStringA name(m_parametersCtrl.GetItemText(hItem));
					size_t pos = m_parametersVariations.Find((LPCSTR)name);
					if (pos < m_parametersVariations.size())
					{
						m_parametersVariations[pos].m_bActive = bChecked;
					}
					else
					{
						//check uncheck all
						//for (size_t i = 0; i < m_parametersVariations.size(); i++)
							//m_parametersVariations[i].m_bActive = bChecked;
					}
				}
			}

			return 1;
		}
		return 0;
	}

	void CParametersVariationsDlg::OnGeneratioTypeChange()
	{
		m_parametersVariations.m_variationType = m_generationTypeCtrl.GetCurSel();
	}

	void CParametersVariationsDlg::OnNbVariationChange()
	{
		m_parametersVariations.m_nbVariation = ToInt(m_nbVariationsCtrl.GetString());
	}
	

	void CParametersVariationsDlg::UpdateCtrl(void)
	{
		if (!m_bShowVariationType)
		{
			m_generationTypeCtrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_CMN_STATIC1)->ShowWindow(SW_HIDE);
			m_nbVariationsCtrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_CMN_STATIC2)->ShowWindow(SW_HIDE);
		}

		bool bEnable = m_generationTypeCtrl.GetCurSel() != CB_ERR;
		m_nbVariationsCtrl.EnableWindow(bEnable);
		GetDlgItem(IDC_CMN_STATIC2)->EnableWindow(bEnable);

	}

	//Do nothing
	void CParametersVariationsDlg::OnOK()
	{}

	void CParametersVariationsDlg::OnCancel()
	{}


	void CParametersVariationsDlg::OnDestroy()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("ParametersVariationsDlg"), pt);

		CDialog::OnDestroy();
	}

	BOOL CParametersVariationsDlg::PreTranslateMessage(MSG* pMsg)
	{
		m_propertiesCtrl.PreTranslateMessage(pMsg);

		return CDialog::PreTranslateMessage(pMsg);
	}


	void CParametersVariationsDlg::OnEnable(BOOL bEnable)
	{
		m_parametersCtrl.EnableWindow(bEnable);
		m_propertiesCtrl.EnableWindow(bEnable);

		GetDlgItem(IDC_CMN_STATIC1)->EnableWindow(bEnable);
		GetDlgItem(IDC_CMN_STATIC2)->EnableWindow(bEnable);
		m_generationTypeCtrl.EnableWindow(bEnable);
		m_nbVariationsCtrl.EnableWindow(bEnable);
		

	}


}
