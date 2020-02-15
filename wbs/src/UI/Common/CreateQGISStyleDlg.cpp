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

#include "UI/Common/CreateQGISStyleDlg.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "CreateQGISStyleDlg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CCreateQGISStyleDlg dialog


	CCreateQGISStyleDlg::CCreateQGISStyleDlg(CWnd* pParentWnd/*=NULL*/) :
		CDialogEx(CCreateQGISStyleDlg::IDD, pParentWnd)
	{
		//{{AFX_DATA_INIT(CCreateQGISStyleDlg)
		//}}AFX_DATA_INIT
	}


	void CCreateQGISStyleDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_MAP_CREATE_QGIS_STYLE, m_createStyle);

		DDX_Control(pDX, IDC_MAP_PALETTE_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_MAP_COLOR_RAMP_TYPE, m_colorRampCtrl);

		DDX_Control(pDX, IDC_MAP_NB_CLASSES, m_nbClassesCtrl);
		DDX_Control(pDX, IDC_MAP_CLASSES_SIZE, m_classesSizeCtrl);
		DDX_Control(pDX, IDC_MAP_VAR_FACTOR, m_varFactorCtrl);

		DDX_Control(pDX, IDC_MAP_USER_MIN, m_userMinCtrl);
		DDX_Control(pDX, IDC_MAP_USER_MAX, m_userMaxCtrl);
		DDX_Control(pDX, IDC_MAP_INVERT_PALETTE, m_reversePaletteCtrl);

		DDX_Control(pDX, IDC_MAP_NUMBER_FORMAT, m_numberFormatCtrl);
		DDX_Control(pDX, IDC_MAP_DATE_FORMAT, m_dateFormatCtrl);


		if (pDX->m_bSaveAndValidate)
		{
			m_options.m_create_style_file = m_createStyle.GetCheck();
			m_options.m_palette_name = m_nameCtrl.GetString();
			m_options.m_breaks_type = GetBreakType();
			m_options.m_nb_classes = m_nbClassesCtrl.GetInt();
			m_options.m_class_size = m_classesSizeCtrl.GetFloat();

			m_options.m_min_max_type = GetMinMaxType();
			m_options.m_var_factor = m_varFactorCtrl.GetFloat();
			m_options.m_min = m_userMinCtrl.GetFloat();
			m_options.m_max = m_userMaxCtrl.GetFloat();
			m_options.m_reverse_palette = m_reversePaletteCtrl.GetCheck();
			m_options.m_color_ramp_type = m_colorRampCtrl.GetCurSel();
			m_options.m_number_format = m_numberFormatCtrl.GetString();
			m_options.m_date_format = m_dateFormatCtrl.GetString();
		}
		else
		{
			m_createStyle.SetTitleStyle();
			m_createStyle.SetGroupID(1);
			m_createStyle.SetCheck(m_options.m_create_style_file);

			InitPaletteNameList();

			m_nameCtrl.SelectStringExact(0, m_options.m_palette_name);
			m_nbClassesCtrl.SetInt(m_options.m_nb_classes);
			m_classesSizeCtrl.SetFloat(m_options.m_class_size);
			m_varFactorCtrl.SetFloat(m_options.m_var_factor);
			m_userMinCtrl.SetFloat(m_options.m_min);
			m_userMaxCtrl.SetFloat(m_options.m_max);
			m_reversePaletteCtrl.SetCheck(m_options.m_reverse_palette);
			m_colorRampCtrl.SetCurSel((int)m_options.m_color_ramp_type);
			m_numberFormatCtrl.SetString(m_options.m_number_format);
			m_dateFormatCtrl.SetString(m_options.m_date_format);

			CheckRadioButton(IDC_MAP_BY_NB_CLASSES, IDC_MAP_BY_CLASSES_SIZE, IDC_MAP_BY_NB_CLASSES + (int)m_options.m_breaks_type);
			CheckRadioButton(IDC_MAP_BY_MINMAX, IDC_MAP_BY_USER, IDC_MAP_BY_MINMAX + (int)m_options.m_min_max_type);
		}
	}



	BEGIN_MESSAGE_MAP(CCreateQGISStyleDlg, CDialogEx)
		ON_BN_CLICKED(IDC_MAP_CREATE_STYLE, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_BY_NB_CLASSES, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_BY_CLASSES_SIZE, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_BY_MINMAX, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_BY_STDDEV, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_BY_USER, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_BN_CLICKED(IDC_MAP_CREATE_QGIS_STYLE, &CCreateQGISStyleDlg::UpdateCtrl)
		ON_NOTIFY(NM_CLICK, IDC_MAP_NUMBER_FORMAT_LABLE, &CCreateQGISStyleDlg::OnGetFormatClick)
		ON_NOTIFY(NM_CLICK, IDC_MAP_DATE_FORMAT_LABLE, &CCreateQGISStyleDlg::OnGetFormatClick)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CCreateQGISStyleDlg msg handlers




	BOOL CCreateQGISStyleDlg::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		//CAppOption option(_T("CreateQGISStyle"));



		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}


	void CCreateQGISStyleDlg::InitPaletteNameList(void)
	{
		ERMsg msg;


		std::string file_path = GetApplicationPath() + "Palette\\palettes.xml";
		CQGISPalettes palettes;
		msg = palettes.load(file_path);
		if (!msg)
		{
			UtilWin::SYShowMessage(msg, this);
			return;
		}

		CString name;
		m_nameCtrl.GetWindowText(name);
		m_nameCtrl.ResetContent();


		for (auto it = palettes.begin(); it != palettes.end(); it++)
			m_nameCtrl.AddString(it->first);

		m_nameCtrl.SelectStringExact(0, name);

	}

	size_t CCreateQGISStyleDlg::GetBreakType()
	{
		return GetCheckedRadioButton(IDC_MAP_BY_NB_CLASSES, IDC_MAP_BY_CLASSES_SIZE) - IDC_MAP_BY_NB_CLASSES;
	}

	size_t CCreateQGISStyleDlg::GetMinMaxType()
	{
		return GetCheckedRadioButton(IDC_MAP_BY_MINMAX, IDC_MAP_BY_USER) - IDC_MAP_BY_MINMAX;
	}


	void CCreateQGISStyleDlg::UpdateCtrl()
	{
		bool bEnable = m_createStyle.GetCheck();
		if (bEnable)
		{
			size_t bType = GetBreakType();
			m_nbClassesCtrl.EnableWindow(bType == CCreateStyleOptions::BY_NB_CLASS);
			m_classesSizeCtrl.EnableWindow(bType == CCreateStyleOptions::BY_CLASS_SIZE);

			size_t mmType = GetMinMaxType();
			m_varFactorCtrl.EnableWindow(mmType == CCreateStyleOptions::BY_STDDEV);
			m_userMinCtrl.EnableWindow(mmType == CCreateStyleOptions::BY_USER);
			m_userMaxCtrl.EnableWindow(mmType == CCreateStyleOptions::BY_USER);
		}
	}


	void CCreateQGISStyleDlg::OnGetFormatClick(NMHDR *pNMHDR, LRESULT *pResult)
	{
		PNMLINK pNMLink = (PNMLINK)pNMHDR;
		ShellExecuteW(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);

		*pResult = 0;
	}



}


