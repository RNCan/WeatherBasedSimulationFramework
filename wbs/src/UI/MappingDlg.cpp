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
#include "MappingDlg.h"

#include "Geomatic/GridInterpol.h"
#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AppOption.h"
#include "UI/DBEditorPropSheet.h"
#include "UI/TransfoDlg.h"
#include "UI/MappingAdvancedDlg.h"


using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CMappingDlg dialog


	CMappingDlg::CMappingDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CMappingDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
		//{{AFX_DATA_INIT(CMappingDlg)
		//}}AFX_DATA_INIT
	}


	void CMappingDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_MAP_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_MAP_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_MAP_METHOD, m_interpolMethodCtrl);
		DDX_Control(pDX, IDC_MAP_DEM_FILENAME, m_DEMCtrl);
		DDX_Control(pDX, IDC_MAP_TEM_FILENAME, m_TEMCtrl);
		DDX_Control(pDX, IDC_MAP_TRANSFO_INFO, m_tranfoInfoCtrl);

		DDX_Control(pDX, IDC_MAP_DEFAULT_DIR, m_defaultDirCtrl);
		DDX_Control(pDX, IDC_MAP_NB_CREATED, m_dimensionCtrl);
		DDX_Control(pDX, IDC_MAP_XVAL_ONLY, m_XValOnlyCtrl);
		DDX_Control(pDX, IDC_MAP_USE_HXGRID, m_useHxGridCtrl);




		if (!pDX->m_bSaveAndValidate)
		{
			CParentInfoFilter filter;
			filter.reset();
			filter.set(VARIABLE);
			filter.set(TIME_REF);
			filter.set(PARAMETER);
			filter.set(REPLICATION);


			m_pParent->GetParentInfo(WBSF::GetFM(), m_parentInfo, filter);

			InitDEMList();
			SetMappingToInterface();
		}
		else
		{
			GetMappingFromInterface();
		}

	}



	BEGIN_MESSAGE_MAP(CMappingDlg, CDialog)
		ON_BN_CLICKED(IDC_MAP_EDITOR, OnMapEditor)
		ON_BN_CLICKED(IDC_MAP_TRANFORM, OnEditTransfo)
		ON_BN_CLICKED(IDC_MAP_ADVANCED, OnEditAdvanced)

	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CMappingDlg msg handlers




	BOOL CMappingDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		CAppOption option(_T("ExecuteCtrl"));
		bool bEnableHxGrid = option.GetProfileInt(_T("UseHxGrid"), FALSE);
		m_useHxGridCtrl.EnableWindow(bEnableHxGrid);
		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}


	void CMappingDlg::InitDEMList(void)
	{

		WBSF::StringVector mapList = WBSF::GetFM().MapInput().GetFilesList();

		CString name;
		m_DEMCtrl.GetWindowText(name);
		m_DEMCtrl.ResetContent();

		for (size_t i = 0; i < mapList.size(); i++)
			m_DEMCtrl.AddString(mapList[i]);

		m_DEMCtrl.SelectStringExact(0, name);

		m_defaultDirCtrl.SetWindowText(WBSF::GetFM().GetOutputMapPath().c_str());

	}

	void CMappingDlg::OnMapEditor()
	{
		CDBManagerDlg dlg(this, CDBManagerDlg::MAP_INPUT);
		dlg.DoModal();

		InitDEMList();

		//CMapEditDlg mapDlg(WBSF::GetFM(), this);
		//mapDlg.m_DEMName = m_DEMCtrl.GetWindowText();

		//if( mapDlg.DoModal() == IDOK )
		//{
		//	InitDEMList();
		//	int sel = m_DEMCtrl.FindStringExact(-1, mapDlg.m_DEMName );
		//	if( sel != CB_ERR ) m_DEMCtrl.SetCurSel(sel);
		//
		//}
	}
	void CMappingDlg::OnEditAdvanced()
	{
		CMappingAdvancedDlg dlg(this);

		*(dlg.m_pParam) = *(m_mapping.m_pParam);
		dlg.m_method = m_interpolMethodCtrl.GetCurSel();

		if (dlg.DoModal() == IDOK)
		{
			*m_mapping.m_pParam = *dlg.m_pParam;
		}
	}


	void CMappingDlg::OnEditTransfo()
	{
		CTransfoDlg dlg(this);

		dlg.SetPrePostTransfo(m_mapping.GetPrePostTransfo());

		if (dlg.DoModal() == IDOK)
		{
			m_mapping.SetPrePostTransfo(dlg.GetPrePostTransfo());
			m_tranfoInfoCtrl.SetWindowText(m_mapping.GetPrePostTransfo().GetDescription());
		}
	}

	void CMappingDlg::UpdateCtrl()
	{
		int p = (int)m_parentInfo.m_parameterset.size();
		int r = (int)m_parentInfo.m_nbReplications;
		int t = (int)m_parentInfo.m_period.GetNbRef();
		int v = (int)m_parentInfo.m_variables.size();

		CString txt;
		txt.Format(_T("%d x %d x %d x %d = %d"), p, r, t, v, p*r*t*v);
		m_dimensionCtrl.SetWindowText(txt);


		GetDlgItem(IDC_MAP_INTERNAL_NAME)->SetWindowText(CString(m_mapping.GetInternalName().c_str()));
	}

	void CMappingDlg::GetMappingFromInterface()
	{
		m_mapping.SetName(m_nameCtrl.GetString());
		m_mapping.SetDescription(m_descriptionCtrl.GetString());
		m_mapping.SetInterpolMethod(m_interpolMethodCtrl.GetCurSel());
		m_mapping.SetDEMName(m_DEMCtrl.GetString());
		m_mapping.SetTEMName(m_TEMCtrl.GetString());
		m_mapping.SetXValOnly(m_XValOnlyCtrl.GetCheck());
		m_mapping.SetUseHxGrid(m_useHxGridCtrl.GetCheck());
	}

	void CMappingDlg::SetMappingToInterface()
	{
		m_nameCtrl.SetWindowText(m_mapping.GetName());
		m_descriptionCtrl.SetWindowText(m_mapping.GetDescription());
		m_interpolMethodCtrl.SetCurSel(m_mapping.GetInterpolMethod());
		m_DEMCtrl.SelectString(-1, m_mapping.GetDEMName());
		m_TEMCtrl.SetWindowText(m_mapping.GetTEMName());
		m_tranfoInfoCtrl.SetWindowText(m_mapping.GetPrePostTransfo().GetDescription());
		m_XValOnlyCtrl.SetCheck(m_mapping.GetXValOnly());
		m_useHxGridCtrl.SetCheck(m_mapping.GetUseHxGrid());
	}

}