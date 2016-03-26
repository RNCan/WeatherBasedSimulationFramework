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
#include "Basic/CSV.h"
#include "FileManager/FileManager.h"
#include "ModelBase/Model.h"
#include "ModelBase/WGInput.h"
#include "UI/Common/UtilWin.h" 
#include "UI/Common/TextDlg.h"
#include "UI/SimulatedAnnealingCtrlDlg.h"
#include "UI/ModelParameterizationDlg.h"
#include "UI/ModelInputManagerDlg.h"
#include "UI/LOCEditDlg.h"

#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


namespace WBSF
{

	//******************************************************************
	//CExecuteDialog

	//******************************************************************
	// CModelParameterizationDlg dialog

	CModelParameterizationDlg::CModelParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(IDD, pParentWnd)
	{
	}


	void CModelParameterizationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SIM_USE_HXGRID, m_useHxGridCtrl);
		DDX_Control(pDX, IDC_SIM_MODEL, m_modelCtrl);
		DDX_Control(pDX, IDC_SIM_RESULT, m_resultFileNameCtrl);
		DDX_Control(pDX, IDC_SIM_LOCID_FIELD, m_locIDFieldCtrl);
		DDX_Control(pDX, IDC_SIM_MODELINPUT, m_modelInputNameCtrl);
		DDX_Control(pDX, IDC_SIM_PARAMETERS_SET, m_parametersVariationsNameCtrl);
		DDX_Control(pDX, IDC_SIM_FEEDBACK, m_feedbackCtrl);
		DDX_Control(pDX, IDC_SIM_REPLICATION, m_replicationCtrl);
	}


	BEGIN_MESSAGE_MAP(CModelParameterizationDlg, CDialog)
		ON_BN_CLICKED(IDC_MODEL_DESCRIPTION, OnModelDescription)
		ON_BN_CLICKED(IDC_SIM_MODELINPUT_EDITOR, OnEditModelInput)
		ON_BN_CLICKED(IDC_SIM_PARAM_VARIATIONS_EDITOR, OnEditParametersVariations)
		ON_BN_CLICKED(IDC_SIM_SA, OnEditSACtrl)
		ON_CBN_SELCHANGE(IDC_SIM_MODEL, OnModelChange)
		ON_CBN_SELCHANGE(IDC_SIM_RESULT, FillLOCIDField)


	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CModelParameterizationDlg message handlers




	void CModelParameterizationDlg::OnOK()
	{
		/*if(m_LOCNameCtrl.GetWindowText().IsEmpty() )
		{
		MessageBox(UtilWin::GetCString(IDS_SIM_NOLOC), AfxGetAppName() , MB_ICONEXCLAMATION|MB_OK);
		return ;
		}*/

		GetSimulationFromInterface();

		WBSF::CRegistry option;
		option.WriteProfileString("LastModel", m_sa.GetModelName().c_str());

		CDialog::OnOK();

		//m_bInit = false;
	}

	BOOL CModelParameterizationDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		//	m_resultFileNameCtrl.EnableFileBrowseButton(".csv","*.csv|*.csv||");


		SetSimulationToInterface();

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}


	void CModelParameterizationDlg::FillModel()
	{
		WBSF::CRegistry option;
		string lastModel = option.GetProfileString("LastModel", "");

		WBSF::StringVector models = WBSF::GetFM().Model().GetFilesList();

		if (models.size() == 0)
		{
			CString error(FormatMsg(IDS_SIM_NOMODELFOUND, WBSF::GetFM().Model().GetLocalPath()).c_str());
			MessageBox(error);
			AfxThrowUserException();
		}

		m_modelCtrl.FillList(models, lastModel);

	}

	void CModelParameterizationDlg::FillModelInput(void)
	{
		ASSERT(m_modelCtrl.GetCurSel() != CB_ERR);

		WBSF::StringVector list = WBSF::GetFM().ModelInput(m_model.GetExtension()).GetFilesList();
		m_modelInputNameCtrl.FillList(list);

		ASSERT(m_modelInputNameCtrl.GetCount() > 0);
	}

	void CModelParameterizationDlg::FillParametersVariations(void)
	{
		WBSF::StringVector list = WBSF::GetFM().PVD(m_model.GetExtension()).GetFilesList();
		m_parametersVariationsNameCtrl.FillList(list);
	}

	void CModelParameterizationDlg::FillResultFile()
	{
		WBSF::StringVector list = WBSF::GetFM().Input().GetFilesList();
		m_resultFileNameCtrl.FillList(list);
	}

	void CModelParameterizationDlg::FillLOCIDField()
	{
		string fileName = m_resultFileNameCtrl.GetString();
		string filePath = WBSF::GetFM().Input().GetFilePath(fileName);

		ifStream file;
		if (file.open(filePath))
		{
			CSVIterator loop(file);
			m_locIDFieldCtrl.FillList(loop.Header());
		}
	}

	void CModelParameterizationDlg::OnModelDescription()
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

	void CModelParameterizationDlg::UpdateCtrl(void)
	{
	}

	void CModelParameterizationDlg::OnModelChange()
	{
		string modelName = m_modelCtrl.GetString();
		if (modelName == m_lastModelLoad)
			return;

		//if( !m_lastModelLoad.empty() )
		//{
		//	//remove all m_sa variation
		//	m_sa.m_parametersVariations.clear();
		//}

		m_lastModelLoad = modelName;
		VERIFY(WBSF::GetFM().Model().Get(modelName, m_model));
		FillModelInput();

		UpdateCtrl();
	}



	void CModelParameterizationDlg::OnEditModelInput()
	{
		CModelInputManagerDlg dlg(this);

		dlg.m_model = m_model;
		dlg.m_modelInputName = m_modelInputNameCtrl.GetWindowText();
		bool bOK = dlg.DoModal() == IDOK;
		FillModelInput();

		if (bOK)
			m_modelInputNameCtrl.SelectStringExact(0, dlg.m_modelInputName);
	}

	void CModelParameterizationDlg::OnEditParametersVariations()
	{
		CParametersVariationsManagerDlg dlg(false, false, this);

		dlg.m_model = m_model;
		dlg.m_parametersVariationsName = m_parametersVariationsNameCtrl.GetWindowText();

		bool bOK = dlg.DoModal() == IDOK;
		FillParametersVariations();

		if (bOK)
			m_parametersVariationsNameCtrl.SelectStringExact(0, dlg.m_parametersVariationsName);


	}


	void CModelParameterizationDlg::OnEditSACtrl()
	{
		CSimulatedAnnealingCtrlDlg dlg;

		dlg.m_ctrl = m_sa.GetControl();

		if (dlg.DoModal() == IDOK)
		{
			m_sa.SetControl(dlg.m_ctrl);
		}

	}

	void CModelParameterizationDlg::GetSimulationFromInterface()
	{
		m_sa.SetName(m_nameCtrl.GetString());
		m_sa.SetDescription(m_descriptionCtrl.GetString());
		m_sa.SetModelName(m_modelCtrl.GetString());

		m_sa.SetModelInputName(m_modelInputNameCtrl.GetString());
		m_sa.m_parametersVariationsName = m_parametersVariationsNameCtrl.GetString();

		m_sa.m_nbReplications = ToShort(m_replicationCtrl.GetString());

		m_sa.SetFeedbackType(m_feedbackCtrl.GetCurSel());
		m_sa.SetUseHxGrid(m_useHxGridCtrl.GetCheck());

		m_sa.SetResultFileName(m_resultFileNameCtrl.GetString());
		m_sa.m_locIDField = m_locIDFieldCtrl.GetCurSel();
	}

	void CModelParameterizationDlg::SetSimulationToInterface()
	{
		m_nameCtrl.SetWindowText(m_sa.GetName());
		m_internalNameCtrl.SetWindowText(m_sa.GetInternalName());
		m_descriptionCtrl.SetWindowText(m_sa.GetDescription());

		m_useHxGridCtrl.SetCheck(m_sa.GetUseHxGrid());
		m_feedbackCtrl.SetCurSel(m_sa.GetFeedbackType());
		m_replicationCtrl.SetString(ToString(m_sa.m_nbReplications));

		//init model comboBox
		FillModel();

		if (!m_sa.GetModelName().empty())
		{
			int pos = m_modelCtrl.SelectStringExact(0, m_sa.GetModelName(), -1);
			if (pos == CB_ERR)
			{
				CString error(WBSF::FormatMsg(IDS_SIM_MODEL_NOTFOUND, m_sa.GetModelName()).c_str());
				MessageBox(error, AfxGetAppName(), MB_ICONEXCLAMATION | MB_OK);
				m_modelCtrl.SetCurSel(0);
			}
		}

		//load model
		OnModelChange();

		//init modelInput comboBox
		FillModelInput();

		int pos = m_modelInputNameCtrl.SelectStringExact(0, m_sa.GetModelInputName(), -1);
		if (pos == CB_ERR)
		{
			CString error(WBSF::FormatMsg(IDS_SIM_INPUTNOTEXIST, m_sa.m_parametersVariationsName).c_str());
			MessageBox(error, AfxGetAppName(), MB_ICONEXCLAMATION | MB_OK);
			m_modelInputNameCtrl.SetCurSel(0);
		}

		FillParametersVariations();

		if (!m_sa.m_parametersVariationsName.empty())
			pos = m_parametersVariationsNameCtrl.SelectStringExact(0, m_sa.m_parametersVariationsName);

		FillResultFile();
		m_resultFileNameCtrl.SelectString(0, m_sa.GetResultFileName());

		FillLOCIDField();
		m_locIDFieldCtrl.SetCurSel((int)m_sa.m_locIDField);
	}

}