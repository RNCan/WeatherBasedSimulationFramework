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
#include "ModelBase/WGInput.h"
#include "ModelBase/Model.h"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h" 
#include "UI/Common/SYShowMessage.h"
#include "UI/ModelInputManagerDlg.h"
#include "UI/ModelExecutionDlg.h"

#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

namespace WBSF
{



	std::string GetTimeSpanStr(CTimeSpan t)
	{
		std::string strTime;

		if (t.GetDays() > 0)
			strTime = UtilWin::ToUTF8(t.Format(_T("%D, %H:%M:%S")));
		else strTime = UtilWin::ToUTF8(t.Format(_T("%H:%M:%S")));

		return strTime;
	}


	//******************************************************************
	// CModelExecutionDlg dialog
	CModelExecutionDlg::CModelExecutionDlg(const CExecutablePtr& pParent, CWnd* pParentWnd /*=NULL*/)
		: CDialog(IDD, pParentWnd),
		m_pParent(pParent)
	{
		m_nbLocations = 0;
		m_nbVariations = 1;

		if (m_pParent)
		{
			m_pParent->GetParentInfo(WBSF::GetFM(), m_info);
		}
	}


	void CModelExecutionDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SIM_HELP, m_helpModelCtrl);
		DDX_Control(pDX, IDC_SIM_ABOUT, m_aboutModelCtrl);
		DDX_Control(pDX, IDC_SIM_MODEL, m_modelCtrl);
		DDX_Control(pDX, IDC_SIM_MODELINPUT, m_modelInputNameCtrl);
		DDX_Control(pDX, IDC_SIM_PARAMETERS_SET, m_parametersVariationsNameCtrl);
		DDX_Control(pDX, IDC_SIM_SEED_TYPE, m_seedCtrl);
		DDX_Control(pDX, IDC_SIM_REPLICATIONS, m_nbReplicationsCtrl);
		DDX_Control(pDX, IDC_SIM_BEHAVIOUR, m_behaviourCtrl);
		DDX_Control(pDX, IDC_SIM_TIME, m_durationCtrl);
		DDX_Control(pDX, IDC_SIM_USE_HXGRID, m_useHxGridCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);


		if (pDX->m_bSaveAndValidate)
			GetModelExecutionFromInterface();
		else
			SetModelExecutionToInterface();

	}


	BEGIN_MESSAGE_MAP(CModelExecutionDlg, CDialog)

		ON_CBN_SELCHANGE(IDC_SIM_MODEL, OnModelChange)
		ON_BN_CLICKED(IDC_SIM_HELP, OnModelHelp)
		ON_BN_CLICKED(IDC_MODEL_DESCRIPTION, OnModelDescription)
		ON_BN_CLICKED(IDC_SIM_MODELINPUT_EDITOR, OnEditModelInput)
		ON_CBN_SELCHANGE(IDC_SIM_MODELINPUT, UpdateCtrl)
		ON_BN_CLICKED(IDC_SIM_PARAM_VARIATIONS_EDITOR, OnEditParametersVariations)
		ON_CBN_SELCHANGE(IDC_SIM_PARAMETERS_SET, OnParametersVariationsChange)
		ON_EN_CHANGE(IDC_SIM_REPLICATIONS, OnReplicationsChange)
		ON_BN_CLICKED(IDC_SIM_USE_HXGRID, UpdateCtrl)


	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CModelExecutionDlg message handlers




	void CModelExecutionDlg::OnOK()
	{


		WBSF::CRegistry option;
		option.WriteProfileString("LastModel", m_modelExecution.m_modelName.c_str());

		CDialog::OnOK();
	}

	BOOL CModelExecutionDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CModelExecutionDlg::FillModel()
	{
		WBSF::CRegistry option;
		string lastModel = option.GetProfileString("LastModel", "");

		//m_modelCtrl.ResetContent();

		WBSF::StringVector models = WBSF::GetFM().Model().GetFilesList();

		if (models.size() == 0)
		{
			CString error(FormatMsg(IDS_SIM_NOMODELFOUND, WBSF::GetFM().Model().GetLocalPath()).c_str());
			MessageBox(error);
			AfxThrowUserException();
		}

		m_modelCtrl.FillList(models, lastModel);
	}

	void CModelExecutionDlg::FillModelInput(void)
	{
		ASSERT(m_modelCtrl.GetCurSel() != CB_ERR);

		WBSF::StringVector list = WBSF::GetFM().ModelInput(m_model.GetExtension()).GetFilesList();
		m_modelInputNameCtrl.FillList(list);
	}

	void CModelExecutionDlg::FillParametersVariations(void)
	{
		ASSERT(m_modelCtrl.GetCurSel() != CB_ERR);

		WBSF::StringVector list = WBSF::GetFM().PVD(m_model.GetExtension()).GetFilesList();
		m_parametersVariationsNameCtrl.FillList(list);
	}


	void CModelExecutionDlg::UpdateCtrl(void)
	{
		WBSF::CRegistry option("ExecuteCtrl");


		bool bEnableHxGrid = option.GetProfileInt("UseHxGrid", FALSE);
		m_useHxGridCtrl.EnableWindow(bEnableHxGrid);

		m_helpModelCtrl.EnableWindow(!m_model.GetHelpFileName().empty());
		m_behaviourCtrl.SetCurSel(m_model.m_behaviour);

		size_t nbReplications = ToSizeT(m_nbReplicationsCtrl.GetString());
		size_t nbRuns = m_info.m_locations.size()*m_info.m_nbReplications*m_nbVariations * nbReplications;


		double TPU = CExecutable::GetExecutionTime(m_modelCtrl.GetString(), m_info.m_period.GetTM(), m_useHxGridCtrl&&bEnableHxGrid);
		double time = TPU*nbRuns*m_info.m_period.GetNbYears();
		string format = WBSF::GetTimeSpanStr(time);

		stringstream s;
		auto loc = std::locale("");
		s.imbue(loc);
		s << nbRuns;
		CString strNbRuns(s.str().c_str());

		SetDlgItemInt(IDC_CMN_STATIC1, (UINT)m_info.m_locations.size());
		SetDlgItemInt(IDC_CMN_STATIC3, (UINT)m_info.m_nbReplications);
		SetDlgItemInt(IDC_CMN_STATIC4, (UINT)m_nbVariations);
		SetDlgItemInt(IDC_CMN_STATIC5, (UINT)nbReplications);
		SetDlgItemText(IDC_SIM_NBRUN, strNbRuns);
		m_durationCtrl.SetWindowText(format);

	}

	void CModelExecutionDlg::OnModelChange()
	{
		ERMsg msg;
		string modelName = m_modelCtrl.GetString();
		if (modelName == m_lastModelLoad)
			return;

		string lastModelInput = m_modelExecution.m_modelInputName;

		if (!m_lastModelLoad.empty())
		{
			//remove all m_modelExecution variation
			m_modelExecution.m_paramVariationsName.clear();
			m_modelExecution.m_modelInputName.clear();
		}

		m_lastModelLoad = modelName;
		VERIFY(WBSF::GetFM().Model().Get(modelName, m_model));

		m_aboutModelCtrl.SetWindowText(m_model.GetDescription());
		m_behaviourCtrl.SetCurSel(m_model.GetBehaviour());

		FillModelInput();

		if (!m_modelExecution.m_modelInputName.empty())
		{
			//init modelInput comboBox
			int pos = m_modelInputNameCtrl.SelectStringExact(0, m_modelExecution.m_modelInputName, -1);
			if (pos == CB_ERR)
			{
				msg.ajoute(FormatMsg(IDS_SIM_INPUTNOTEXIST, m_modelExecution.m_modelInputName));
				m_modelInputNameCtrl.SetCurSel(0);
			}
		}
		else if (!lastModelInput.empty())
		{
			//try to load the same input paramter name event if it's not the same model. 
			m_modelInputNameCtrl.SelectStringExact(0, m_modelExecution.m_modelInputName, 0);
		}

		//init parameters variations comboBox
		FillParametersVariations();
		if (!m_modelExecution.m_paramVariationsName.empty())
		{
			int pos = m_parametersVariationsNameCtrl.SelectStringExact(0, m_modelExecution.m_paramVariationsName, -1);
			if (pos == CB_ERR)
			{
				msg.ajoute(FormatMsg(IDS_SIM_LOCNOTEXIST, m_modelExecution.m_paramVariationsName));
				m_parametersVariationsNameCtrl.SetCurSel(0);
			}
		}

		OnParametersVariationsChange();

		UpdateCtrl();

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}


	void CModelExecutionDlg::OnParametersVariationsChange()
	{
		m_nbVariations = 1;
		string name = m_parametersVariationsNameCtrl.GetString();
		if (!name.empty())
		{
			CParametersVariationsDefinition PVD;
			WBSF::GetFM().PVD().Get(name, PVD);
			m_nbVariations = PVD.GetNbVariation();
		}

		UpdateCtrl();
	}

	void CModelExecutionDlg::OnReplicationsChange()
	{
		UpdateCtrl();
	}





	void CModelExecutionDlg::OnEditModelInput()
	{
		CModelInputManagerDlg dlg(this);

		dlg.m_model = m_model;
		dlg.m_modelInputName = m_modelInputNameCtrl.GetWindowText();

		bool bOk = dlg.DoModal() == IDOK;
		FillModelInput();

		if (bOk)
			m_modelInputNameCtrl.SelectStringExact(0, dlg.m_modelInputName);

	}

	void CModelExecutionDlg::OnEditParametersVariations()
	{
		CParametersVariationsManagerDlg dlg(this);

		dlg.m_model = m_model;
		dlg.m_parametersVariationsName = m_modelExecution.m_paramVariationsName.c_str();

		bool bOk = dlg.DoModal() == IDOK;
		FillParametersVariations();

		if (bOk)
		{
			m_modelExecution.m_paramVariationsName = CStringA(dlg.m_parametersVariationsName);
			m_parametersVariationsNameCtrl.SelectStringExact(0, dlg.m_parametersVariationsName);
			//UpdateCtrl();
		}
			
		OnParametersVariationsChange();
	}


	void CModelExecutionDlg::OnModelHelp()
	{
		ASSERT(!m_model.GetHelpFileName().empty());
		string argument = WBSF::GetFM().Model().GetFilePath(m_model.GetHelpFileName());
		ShellExecuteW(m_hWnd, L"open", CString(argument.c_str()), NULL, NULL, SW_SHOW);
	}

	void CModelExecutionDlg::OnModelDescription()
	{
		string path(WBSF::GetUserDataPath() + "BioSIM\\tmp\\");
		WBSF::CreateMultipleDir(path);
		string filePath(path + m_model.GetName() + ".txt");

		ofStream file;
		if (file.open(filePath))
		{
			file.write(m_model.GetDocumentation());
			file.close();

			ShellExecuteW(m_hWnd, L"open", CStringW(filePath.c_str()), NULL, NULL, SW_SHOW);
		}
	}

	void CModelExecutionDlg::GetModelExecutionFromInterface()
	{
		m_modelExecution.m_name = m_nameCtrl.GetString();
		m_modelExecution.m_description = m_descriptionCtrl.GetString();
		m_modelExecution.m_modelName = m_modelCtrl.GetString();

		m_modelExecution.m_modelInputName = m_modelInputNameCtrl.GetString();
		m_modelExecution.m_paramVariationsName = m_parametersVariationsNameCtrl.GetString();

		m_modelExecution.m_seedType = m_seedCtrl.GetCurSel();
		m_modelExecution.m_nbReplications = std::stoi(m_nbReplicationsCtrl.GetString());
		m_modelExecution.m_bUseHxGrid = m_useHxGridCtrl.GetCheck();
	}

	void CModelExecutionDlg::SetModelExecutionToInterface()
	{
		ERMsg msg;

		m_nameCtrl.SetWindowText(m_modelExecution.GetName());
		m_internalNameCtrl.SetWindowText(m_modelExecution.GetInternalName());
		m_descriptionCtrl.SetWindowText(m_modelExecution.GetDescription());

		//init model comboBox
		FillModel();

		if (!m_modelExecution.m_modelName.empty())
		{
			int pos = m_modelCtrl.SelectStringExact(0, m_modelExecution.m_modelName.c_str(), -1);
			if (pos == CB_ERR)
			{
				msg.ajoute(FormatMsg(IDS_SIM_MODEL_NOTFOUND, m_modelExecution.m_modelName));
				m_modelCtrl.SetCurSel(0);
			}
		}

		//load model
		OnModelChange();


		m_seedCtrl.SetCurSel((int)m_modelExecution.m_seedType);
		m_nbReplicationsCtrl.SetWindowText(std::to_string(m_modelExecution.m_nbReplications));

		//m_weatherLocCtrl.SetCheck(m_modelExecution.GetWeatherLoc() );
		//m_XValidationCtrl.SetCheck(m_modelExecution.GetWeatherLoc()&&m_modelExecution.GetXValidation() );
		m_useHxGridCtrl.SetCheck(m_modelExecution.m_bUseHxGrid);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}

}