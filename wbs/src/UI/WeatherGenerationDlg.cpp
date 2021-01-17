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

#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "ModelBase/Model.h"
#include "ModelBase/WGInput.h"
#include "UI/Common/UtilWin.h" 
#include "UI/Common/SYShowMessage.h"
#include "UI/ParametersVariationsDlg.h"
#include "UI/ModelInputManagerDlg.h"
#include "UI/WeatherGenerationDlg.h"
#include "UI/LOCEditDlg.h"

#include "WeatherBasedSimulationString.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


namespace WBSF
{


	//******************************************************************
	// CWeatherGenerationDlg dialog
	CWeatherGenerationDlg::CWeatherGenerationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd)
		: CDialog(IDD, pParentWnd),
		m_pParent(pParent)
	{
		m_nbLocation = 0;
		m_nbVariations = 1;
	}


	void CWeatherGenerationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SIM_WGINPUT, m_WGInputNameCtrl);
		DDX_Control(pDX, IDC_SIM_LOC, m_locationsNameCtrl);
		DDX_Control(pDX, IDC_SIM_REPLICATIONS, m_replicationsCtrl);

		//DDX_Control(pDX, IDC_SIM_USE_HXGRID, m_useHxGridCtrl);
		//DDX_Control(pDX, IDC_SIM_WEATHER_LOC, m_weatherLocCtrl);
		//DDX_Control(pDX, IDC_SIM_XVALIDATION, m_XValidationCtrl);
		DDX_Control(pDX, IDC_SIM_BEHAVIOUR, m_behaviourCtrl);
		DDX_Control(pDX, IDC_SIM_TIME, m_durationCtrl);


		if (pDX->m_bSaveAndValidate)
			GetWGFromInterface();
		else
			SetWGToInterface();

	}


	BEGIN_MESSAGE_MAP(CWeatherGenerationDlg, CDialog)

		ON_BN_CLICKED(IDC_SIM_WGINPUT_EDITOR, OnEditWGInput)
		ON_BN_CLICKED(IDC_SIM_LOC_EDITOR, OnEditLocations)
		ON_CBN_SELCHANGE(IDC_SIM_WGINPUT, UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_SIM_LOC, OnLocationsChange)
		ON_EN_CHANGE(IDC_SIM_REPLICATIONS, UpdateCtrl)
		ON_BN_CLICKED(IDC_SIM_USE_HXGRID, UpdateCtrl) 

	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CWeatherGenerationDlg message handlers




	void CWeatherGenerationDlg::OnOK()
	{
		if (m_locationsNameCtrl.GetWindowText().IsEmpty())
		{
			MessageBox(UtilWin::GetCString(IDS_SIM_NOLOC), AfxGetAppName(), MB_ICONEXCLAMATION | MB_OK);
			return;
		}


		CDialog::OnOK();

	}

	BOOL CWeatherGenerationDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		UpdateNbLocations();
		//UpdateNbParametersVariations();
		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CWeatherGenerationDlg::FillWGInput(void)
	{
		WBSF::StringVector list = WBSF::GetFM().WGInput().GetFilesList();
		m_WGInputNameCtrl.FillList(list);
	}


	void CWeatherGenerationDlg::FillLocations(void)
	{
		WBSF::StringVector list = WBSF::GetFM().Loc().GetFilesList();
		m_locationsNameCtrl.FillList(list);
	}

	
	void CWeatherGenerationDlg::UpdateCtrl(void)
	{
		WBSF::CRegistry option("ExecuteCtrl");

		//bool bEnableHxGrid = option.GetProfileInt("UseHxGrid", FALSE);
		//m_useHxGridCtrl.EnableWindow(bEnableHxGrid);



		//int nbVariation = m_WG.GetNbParamVariation();
		size_t nbReplications = max(1, ToInt(m_replicationsCtrl.GetString()));

		size_t nbRuns = m_nbLocation* m_nbVariations * nbReplications;

		CString strNbRuns;
		CString tmp = UtilWin::ToCString(nbRuns);
		int n = (tmp.GetLength()) % 3;
		strNbRuns = tmp.Left(n);
		strNbRuns += " ";
		while (tmp.GetLength() > n)
		{
			strNbRuns += tmp.Mid(n, 3);
			strNbRuns += " ";
			n += 3;
		}


		SetDlgItemText(IDC_SIM_NBRUN, strNbRuns);
		SetDlgItemInt(IDC_CMN_STATIC1, (UINT)m_nbLocation);
		SetDlgItemInt(IDC_CMN_STATIC2, (UINT)m_nbVariations);
		SetDlgItemInt(IDC_CMN_STATIC3, (UINT)nbReplications);

		CWGInput WGInput;

		string WGName = m_WGInputNameCtrl.GetString();
		if (!WBSF::GetFM().WGInput().Get(WGName, WGInput))
			WGInput.LoadDefaultParameter();

		bool bDeterminist = (WGInput.UseDaily()|| WGInput.UseHourly()) && (WGInput.m_lastYear < CTRef::GetCurrentTRef().GetYear());
		m_behaviourCtrl.SetCurSel(bDeterminist ? 0 : 1);

		double time = nbRuns*WGInput.GetNbYears()*CExecutable::GetExecutionTime("WeatherGenerator", WGInput.GetTM(), false);
		string format = WBSF::GetTimeSpanStr(time);
		m_durationCtrl.SetWindowText(format);
	}

	void CWeatherGenerationDlg::OnLocationsChange()
	{
		UpdateNbLocations();
		UpdateCtrl();
	}

	//void CWeatherGenerationDlg::OnParametersVariationsChange()
	//{
	//	//UpdateNbParametersVariations();
	//	UpdateCtrl();
	//}



	void CWeatherGenerationDlg::OnEditWGInput()
	{
		CWGInputManagerDlg dlg(this);
		dlg.m_WGInputName = m_WGInputNameCtrl.GetWindowText();

		if (dlg.DoModal() == IDOK)
		{
			FillWGInput();
			m_WGInputNameCtrl.SelectStringExact(0, dlg.m_WGInputName);
		}
		else
		{
			FillWGInput();
		}

		UpdateCtrl();
	}

	void CWeatherGenerationDlg::OnEditLocations()
	{

		CLocationsFileManagerDlg dlg(this);

		dlg.m_locName = m_locationsNameCtrl.GetWindowText();
		BOOL bOK = dlg.DoModal() == IDOK;
		FillLocations();


		if (bOK)
		{
			m_locationsNameCtrl.SelectStringExact(0, dlg.m_locName);
			UpdateNbLocations();
			UpdateCtrl();
		}
	}


	
	static std::string GetTimeSpanStr(CTimeSpan t)
	{
		std::string strTime;

		if (t.GetDays() > 0)
			strTime = UtilWin::ToUTF8(t.Format(_T("%D, %H:%M:%S")));
		else strTime = UtilWin::ToUTF8(t.Format(_T("%H:%M:%S")));

		return strTime;
	}

	void CWeatherGenerationDlg::UpdateNbLocations()
	{
		string name = m_locationsNameCtrl.GetString();

		CLocationVector locations;
		WBSF::GetFM().Loc().Get(name, locations);
		m_nbLocation = locations.size();

	}

	/*void CWeatherGenerationDlg::UpdateNbParametersVariations()
	{
		string name = m_locationsNameCtrl.GetString();

		CParametersVariationsDefinition PVD;
		WBSF::GetFM().PVD().Get(name, PVD);
		m_nbLocation = PVD.GetNbVariation();

	}
*/





	void CWeatherGenerationDlg::GetWGFromInterface()
	{
		m_WG.m_name = m_nameCtrl.GetString();
		m_WG.m_description = m_descriptionCtrl.GetString();

		m_WG.m_WGInputName = m_WGInputNameCtrl.GetString();
		m_WG.m_locationsName = m_locationsNameCtrl.GetString();
		m_WG.m_nbReplications = ToInt(m_replicationsCtrl.GetString());
		//m_WG.m_bUseHxGrid = m_useHxGridCtrl.GetCheck();


		if (m_WG.m_nbReplications < 1)
			m_WG.m_nbReplications = 1;

		//m_WG.SetWeatherLoc( m_weatherLocCtrl.GetCheck() );
		//m_WG.SetXValidation( m_weatherLocCtrl.GetCheck()&&m_XValidationCtrl.GetCheck() );

	}

	void CWeatherGenerationDlg::SetWGToInterface()
	{
		ERMsg error;

		m_nameCtrl.SetWindowText(m_WG.GetName());
		m_internalNameCtrl.SetWindowText(m_WG.GetInternalName());
		m_descriptionCtrl.SetWindowText(m_WG.GetDescription());

		//init LOC comboBox
		FillLocations();

		if (!m_WG.m_locationsName.empty())
		{
			if (m_locationsNameCtrl.SelectStringExact(0, m_WG.m_locationsName) == CB_ERR)
			{
				error.ajoute(FormatMsg(IDS_SIM_LOCNOTEXIST, m_WG.m_locationsName));
				m_locationsNameCtrl.SetCurSel(0);
			}
		}

		//init TG input
		FillWGInput();
		if (m_WGInputNameCtrl.SelectStringExact(0, m_WG.m_WGInputName) == CB_ERR)
		{
			error.ajoute(FormatMsg(IDS_SIM_INPUTNOTEXIST, m_WG.m_WGInputName));
			m_WGInputNameCtrl.SetCurSel(0);
		}
		
		m_replicationsCtrl.SetString(to_string(m_WG.m_nbReplications));
		

		if (!error)
			UtilWin::SYShowMessage(error, this);
	}

}