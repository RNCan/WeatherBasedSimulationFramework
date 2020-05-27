//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-04-2020	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"

#include "ModelBase/DevRateEquation.h"
#include "ModelBase/MortalityEquation.h"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/FileNameProperty.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/FitEquationOptionDlg.h"


#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace std;

namespace WBSF
{


	//typedef CStdIndexProperty < IDS_SIM_DISPERSAL_WT> CWeatherTypeProperty;
	//typedef CStdIndexProperty < IDS_SIM_SEED_TYPE> CSeedTypeProperty;
	//typedef CStdIndexProperty < IDS_SIM_BROOD_TYPE> CBroodTypeProperty;
	//typedef CStdIndexProperty < IDS_SIM_PRCP_TYPE> CPrcpTypeProperty;

	
	//*****************************************************************************************************
	
	BEGIN_MESSAGE_MAP(CFitInputParamPropertyGridCtrl, CMFCPropertyGridCtrl)
		//ON_WM_CREATE()
		
	END_MESSAGE_MAP()

	CFitInputParamPropertyGridCtrl::CFitInputParamPropertyGridCtrl():
		m_eq_options(nullptr)
	{
	}

	void CFitInputParamPropertyGridCtrl::Init()
	{
		CMFCPropertyGridCtrl::Init();

		CRect rect;
		GetClientRect(rect);
		//add all attribute
		CAppOption options;
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
		float ratio = options.GetProfileFloat(_T("EqOptionsPropertiesSplitterPos"), 0.5);
		m_nLeftColumnWidth = ratio * rect.Width();
		//SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));

		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
		//EnableDescriptionArea(false);
		SetVSDotNetLook(true);
		MarkModifiedProperties(true);
		SetAlphabeticMode(false);
		SetShowDragContext(true);
		EnableWindow(true);
		AlwaysShowUserToolTip();

		RemoveAll();

		//General 
		//for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
		//{
		//	TDevRateEquation eq = CDevRateEquation::eq(e);
		//	string e_name = CDevRateEquation::GetEquationName(eq);
		//	CSAParameterVector params = CDevRateEquation::GetParameters(eq);
		//	//CSAParameterVector params = params_0;
		//	if (m_eq_options->find(e_name) != m_eq_options->end())
		//	{
		//		if ((*m_eq_options)[e_name].size() == CDevRateEquation::GetParameters(eq).size())
		//			params = (*m_eq_options)[e_name];
		//	}
		//	//CDevRateEquation::GetEquationR(e)
		//	CMFCPropertyGridProperty* pInput = new CMFCPropertyGridProperty(CString(e_name.c_str()), -1);
		//	for (size_t p = 0; p < params.size(); p++)
		//	{

		//		//string value_0 = FormatA("%.7g", params_0[p].m_initialValue) + params_0[p].m_bounds.ToString();
		//		//CMFCPropertyGridProperty* pItem = new CMFCPropertyGridProperty(CString(params[p].m_name.c_str()), CString(value.c_str()), _T(""), ee * 10 + p);
		//		string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
		//		CStdGridProperty* pItem = new CStdGridProperty(params[p].m_name, value.c_str(), "", e * 10 + p);
		//		pInput->AddSubItem(pItem);
		//	}

		//	AddProperty(pInput, FALSE, FALSE);
		//}

		//ExpandAll(FALSE);
		//AdjustLayout();
	}
	
	

	void CFitInputParamPropertyGridCtrl::ResetDefault()
	{
		//General 
		/*for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
		{
			TDevRateEquation eq = CDevRateEquation::eq(e);
			string e_name = CDevRateEquation::GetEquationName(eq);
			CSAParameterVector params = CDevRateEquation::GetParameters(eq);
			
			
			CMFCPropertyGridProperty* pInput = GetProperty((int)e);
			ASSERT(pInput->GetSubItemsCount()== params.size());
			for (size_t p = 0; p < params.size(); p++)
			{
				string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
				CMFCPropertyGridProperty* pItem = pInput->GetSubItem((int)p);
				pItem->SetOriginalValue(CString(value.c_str()));
				pItem->ResetOriginalValue();
			}
		}*/
	}

	void CFitInputParamPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CFitInputParamPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CFitInputParamPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
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

	BOOL CFitInputParamPropertyGridCtrl::ValidateItemData(CMFCPropertyGridProperty* /*pProp*/)
	{
		return TRUE;
	}


	//**************************************************************************************************************

	//**********************************************************************************************
	void CFitInputParamDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_PROPERTIES, m_propertiesCtrl);

		if (pDX->m_bSaveAndValidate)
		{
		}
		else
		{
			//resize window
			CRect rectClient;
			GetWindowRect(rectClient);

			CAppOption option;
			rectClient = option.GetProfileRect(_T("EqOptionsDlgRect"), rectClient);
			UtilWin::EnsureRectangleOnDisplay(rectClient);
			SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);



			//General 
			if (m_fitType == F_DEV_RATE)
			{
				for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
				{
					TDevRateEquation eq = CDevRateEquation::eq(e);
					string e_name = CDevRateEquation::GetEquationName(eq);
					CSAParameterVector params = CDevRateEquation::GetParameters(eq);
					//CSAParameterVector params = params_0;
					if (m_eq_options.find(e_name) != m_eq_options.end())
					{
						if ((m_eq_options)[e_name].size() == CDevRateEquation::GetParameters(eq).size())
							params = (m_eq_options)[e_name];
					}
					//CDevRateEquation::GetEquationR(e)
					CMFCPropertyGridProperty* pInput = new CMFCPropertyGridProperty(CString(e_name.c_str()), -1);
					for (size_t p = 0; p < params.size(); p++)
					{

						//string value_0 = FormatA("%.7g", params_0[p].m_initialValue) + params_0[p].m_bounds.ToString();
						//CMFCPropertyGridProperty* pItem = new CMFCPropertyGridProperty(CString(params[p].m_name.c_str()), CString(value.c_str()), _T(""), ee * 10 + p);
						string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
						CStdGridProperty* pItem = new CStdGridProperty(params[p].m_name, value.c_str(), "", e * 10 + p);
						pInput->AddSubItem(pItem);
					}

					m_propertiesCtrl.AddProperty(pInput, FALSE, FALSE);
				}
			}
			else if (m_fitType == F_MORTALITY)
			{
				for (size_t e = 0; e < CMortalityEquation::NB_EQUATIONS; e++)
				{
					TMortalityEquation eq = CMortalityEquation::eq(e);
					string e_name = CMortalityEquation::GetEquationName(eq);
					CSAParameterVector params = CMortalityEquation::GetParameters(eq);
					//CSAParameterVector params = params_0;
					if (m_eq_options.find(e_name) != m_eq_options.end())
					{
						if ((m_eq_options)[e_name].size() == CMortalityEquation::GetParameters(eq).size())
							params = (m_eq_options)[e_name];
					}
					//CDevRateEquation::GetEquationR(e)
					CMFCPropertyGridProperty* pInput = new CMFCPropertyGridProperty(CString(e_name.c_str()), -1);
					for (size_t p = 0; p < params.size(); p++)
					{
						string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
						CStdGridProperty* pItem = new CStdGridProperty(params[p].m_name, value.c_str(), "", e * 10 + p);
						pInput->AddSubItem(pItem);
					}

					m_propertiesCtrl.AddProperty(pInput, FALSE, FALSE);
				}
			}

			m_propertiesCtrl.ExpandAll(FALSE);
			m_propertiesCtrl.AdjustLayout();
			//m_propertiesCtrl.ExpandAll(FALSE);
		}
	}

	//IMPLEMENT_DYNAMIC(CFitInputParamDlg, CDialogEx)

	BEGIN_MESSAGE_MAP(CFitInputParamDlg, CDialogEx)
		ON_WM_SIZE()
		ON_WM_DESTROY()
		ON_BN_CLICKED(ID_RESET_DEFAULT, &OnBnClickedResetDefault)
		ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
	END_MESSAGE_MAP()



	CFitInputParamDlg::CFitInputParamDlg(size_t type, CWnd* pParentDlg) :
		CDialogEx(IDD, pParentDlg),
		m_fitType(type)
	{
		m_propertiesCtrl.Set(&m_eq_options);
	}


	CFitInputParamDlg::~CFitInputParamDlg()
	{
	}

	void CFitInputParamDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialogEx::OnSize(nType, cx, cy);

		AdjustLayout();
	}

	void CFitInputParamDlg::AdjustLayout()
	{
		static const int MARGE = 10;
		if (GetSafeHwnd() == NULL || m_propertiesCtrl.GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient;
		GetClientRect(rectClient);

		CRect rectOK;
		GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);

		rectOK.MoveToX(rectClient.right - 2 * MARGE - 2 * rectOK.Width());
		rectOK.MoveToY(rectClient.bottom - MARGE - rectOK.Height());

		CRect rectCancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
		rectCancel.left = rectClient.right - MARGE - rectCancel.Width();
		rectCancel.top = rectClient.bottom - MARGE - rectCancel.Height();

		CRect rectReset;
		GetDlgItem(ID_RESET_DEFAULT)->GetWindowRect(rectReset); ScreenToClient(rectReset);
		//rectReset.left = rectReset.right - MARGE - rectReset.Width();
		rectReset.top = rectClient.bottom - MARGE - rectReset.Height();

		
		
		CRect rect;
		m_propertiesCtrl.GetWindowRect(rect); ScreenToClient(rect);
		rect.right = rectClient.right - MARGE;
		rect.bottom = rectClient.bottom - MARGE - rectOK.Height() - MARGE;

		m_propertiesCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(ID_RESET_DEFAULT)->SetWindowPos(NULL, rectReset.left, rectReset.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}


	void CFitInputParamDlg::OnDestroy()
	{
		CRect rectClient;
		GetWindowRect(rectClient);

		CAppOption option;
		option.WriteProfileRect(_T("EqOptionsDlgRect"), rectClient);

		CDialogEx::OnDestroy();
	}

	void CFitInputParamDlg::OnBnClickedResetDefault()
	{
		m_eq_options.clear();

		if (m_fitType == F_DEV_RATE)
		{

			for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
			{
				TDevRateEquation eq = CDevRateEquation::eq(e);
				string e_name = CDevRateEquation::GetEquationName(eq);
				CSAParameterVector params = CDevRateEquation::GetParameters(eq);


				CMFCPropertyGridProperty* pInput = m_propertiesCtrl.GetProperty((int)e);
				ASSERT(pInput->GetSubItemsCount() == params.size());
				for (size_t p = 0; p < params.size(); p++)
				{
					string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
					CMFCPropertyGridProperty* pItem = pInput->GetSubItem((int)p);
					pItem->SetOriginalValue(CString(value.c_str()));
					pItem->ResetOriginalValue();
				}
			}
		}
		else if (m_fitType == F_MORTALITY)
		{
			for (size_t e = 0; e < CMortalityEquation::NB_EQUATIONS; e++)
			{
				TMortalityEquation eq = CMortalityEquation::eq(e);
				string e_name = CMortalityEquation::GetEquationName(eq);
				CSAParameterVector params = CMortalityEquation::GetParameters(eq);


				CMFCPropertyGridProperty* pInput = m_propertiesCtrl.GetProperty((int)e);
				ASSERT(pInput->GetSubItemsCount() == params.size());
				for (size_t p = 0; p < params.size(); p++)
				{
					string value = FormatA("%.7g", params[p].m_initialValue) + params[p].m_bounds.ToString();
					CMFCPropertyGridProperty* pItem = pInput->GetSubItem((int)p);
					pItem->SetOriginalValue(CString(value.c_str()));
					pItem->ResetOriginalValue();
				}
			}
		}
		//m_propertiesCtrl.ResetDefault();
	}


	//void CFitInputParamPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	LRESULT CFitInputParamDlg::OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam)
	{
		CMFCPropertyGridProperty* pPropIn = (CMFCPropertyGridProperty*)lparam;
		
		CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		std::string str = pProp->get_string();

		size_t i = (size_t)pProp->GetData();
		size_t e = size_t(i / 10);
		size_t p = i - e * 10;

		std::string e_name;
		if (m_fitType == F_DEV_RATE)
		{
			TDevRateEquation eq = CDevRateEquation::eq(e);
			ASSERT(e < CDevRateEquation::NB_EQUATIONS);
			ASSERT(p < CDevRateEquation::GetParameters(eq).size());

			e_name = CDevRateEquation::GetEquationName(eq);
			if (m_eq_options.find(e_name) == m_eq_options.end())
				m_eq_options[e_name] = CDevRateEquation::GetParameters(eq);

		}
		else if (m_fitType == F_MORTALITY)
		{
			TMortalityEquation eq = CMortalityEquation::eq(e);
			ASSERT(e < CMortalityEquation::NB_EQUATIONS);
			ASSERT(p < CMortalityEquation::GetParameters(eq).size());

			e_name = CMortalityEquation::GetEquationName(eq);
			if (m_eq_options.find(e_name) == m_eq_options.end())
				m_eq_options[e_name] = CMortalityEquation::GetParameters(eq);

		}
		

		StringVector tmp(str, "[,]|");
		if (tmp.size() == 3)
		{
			string p_name = m_eq_options[e_name][p].m_name;
			m_eq_options[e_name][p] = CSAParameter(p_name, stod(tmp[0]), stod(tmp[1]), stod(tmp[2]));
		}

		return 0;
	}

}

