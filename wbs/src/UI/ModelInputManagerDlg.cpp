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


#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AppOption.h"


#include "ModelInputDlg.h"
#include "ModelInputManagerDlg.h"


using namespace UtilWin;
using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	CModelInputManagerDlg::CModelInputManagerDlg(CWnd* pParent) :
		CDialog(CModelInputManagerDlg::IDD, pParent)
	{
	}

	CModelInputManagerDlg::~CModelInputManagerDlg()
	{}


	void CModelInputManagerDlg::DoDataExchange(CDataExchange* pDX)
	{
		m_fileListCtrl.m_model = m_model;

		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELIN_LIST, m_fileListCtrl);
		DDX_Control(pDX, IDC_MODELIN_FILEPATH, m_filePathCtrl);
	}


	BEGIN_MESSAGE_MAP(CModelInputManagerDlg, CDialog)
		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_MODELIN_LIST, OnSelChange)
		ON_NOTIFY(ON_BLB_NAMECHANGE, IDC_MODELIN_LIST, OnSelChange)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CWGInputManagerDlg msg handlers

	BOOL CModelInputManagerDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = option.GetProfilePoint(_T("ModelInputManager"), CPoint(30, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		//select current tg input
		m_fileListCtrl.SelectString(m_modelInputName);

		UpdateCtrl();
		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}


	void CModelInputManagerDlg::UpdateCtrl()
	{
		BOOL bEnable = m_fileListCtrl.GetSelectedCount() == 1;

		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);

		int iItem = m_fileListCtrl.GetSelItem();
		string name = ToUTF8(m_fileListCtrl.GetItemText(iItem));

		string filePath = m_fileListCtrl.IsDefault() ? GetFM().ModelInput().GetLocalPath() : GetFM().ModelInput().GetFilePath(name);
		m_filePathCtrl.SetWindowText(Convert(filePath));

	}

	void CModelInputManagerDlg::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	{
		UpdateCtrl();
	}

	void CModelInputManagerDlg::OnOK()
	{
		ERMsg msg;
		ASSERT(m_fileListCtrl.GetSelectedCount() == 1);

		m_modelInputName.Empty();
		if (!m_fileListCtrl.IsDefault())
		{
			m_modelInputName = m_fileListCtrl.GetWindowText();
			msg = m_fileListCtrl.SaveModelInput(m_modelInputName, false);

			if (!msg)
			{
				SYShowMessage(msg, this);
				return;
			}
		}
		CDialog::OnOK();
	}

	//void CModelInputManagerDlg::OnCancel()
	//{
	//	//Do nothing
	//}

	BOOL CModelInputManagerDlg::DestroyWindow()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("ModelInputManager"), pt);

		return CDialog::DestroyWindow();
	}


	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CModelInputListBox, CBioSIMListBox)
		//ON_WM_PAINT()
	END_MESSAGE_MAP()



	CModelInputListBox::CModelInputListBox() :CBioSIMListBox(true)
		//CBioSIMListBox(new CModelInputDirectoryManager(GetFM().ModelInput()), true)
	{
		m_pModelInputDlg = new CModelInputDlg;
		m_lastSelection = -1;
	}

	CModelInputListBox::~CModelInputListBox()
	{
		ASSERT(m_pModelInputDlg);

		if (m_pModelInputDlg->m_hWnd != NULL)
			m_pModelInputDlg->DestroyWindow();

		delete m_pModelInputDlg;
		m_pModelInputDlg = NULL;
	}

	CWnd* CModelInputListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_SET_DEFAULT);
		OnInitList();

		m_pModelInputDlg->m_appPath = GetFM().GetAppPath();
		m_pModelInputDlg->m_projectPath = GetFM().GetProjectPath();
		m_pModelInputDlg->Create(m_model, this);

		return m_pWndList;
	}


	void CModelInputListBox::OnAfterAddItem(int iItem)
	{

		string modelInputName = ToUTF8(GetItemText(iItem));

		CModelInput modelInput;
		m_model.GetDefaultParameter(modelInput);

		ERMsg msg = GetFM().ModelInput(m_model.GetExtension()).Set(modelInputName, modelInput);
		if (msg)
		{
			m_pModelInputDlg->SetModelInput(modelInput, iItem <= 0);
			m_lastSelection = iItem;
			m_lastModelInput = modelInput;
		}
		else
		{
			RemoveItem(iItem);
			SYShowMessage(msg, this);
		}
	}

	BOOL CModelInputListBox::OnBeforeCopyItem(int iItem, CString newName)
	{
		BOOL bRep = CBioSIMListBox::OnBeforeCopyItem(iItem, newName);
		if (bRep)
		{
			//m_lastSelection = iItem;//By RSA 2019-01-24
			//CString modelName = newName;

			CModelInput modelInput;
			LoadModelInput(newName, modelInput);
			m_pModelInputDlg->SetModelInput(modelInput, m_lastSelection <= 0);
			m_lastModelInput = modelInput;
		}

		return bRep;
	}

	void CModelInputListBox::OnSetAsDefault(int iItem)
	{
		if (iItem > 0)
		{
			CString modelName = GetItemText(iItem);

			if (!SaveModelInput(modelName, false))
				return;


			if (AfxMessageBox(IDS_DB_SET_AS_DEFAULT_CONFIRM, MB_YESNOCANCEL) == IDYES)
			{
				CModelInput modelInput;
				VERIFY(LoadModelInput(modelName, modelInput));
				m_model.SetDefaultParameter(modelInput);
			}
		}
	}


	void CModelInputListBox::OnSelectionChanged()
	{
		ERMsg msg;

		//event if they have error, we don't take care of
		if (m_lastSelection > 0)
		{
			SaveModelInput(GetItemText(m_lastSelection));
		}

		m_lastSelection = GetSelItem();
		CString modelName = GetItemText(m_lastSelection);

		CModelInput modelInput;
		msg = LoadModelInput(modelName, modelInput);
		m_pModelInputDlg->SetModelInput(modelInput, m_lastSelection <= 0);
		


		m_lastModelInput = modelInput;

		if (!msg)
			SYShowMessage(msg, this);
		

		

		//call parent
		CBioSIMListBox::OnSelectionChanged();
	}

	ERMsg CModelInputListBox::LoadModelInput(const CString& curName, CModelInput& modelInput)
	{
		ERMsg msg;
		if (curName.IsEmpty() || curName == STRDEFAULT)
		{
			m_model.GetDefaultParameter(modelInput);
		}
		else
		{
			msg = GetFM().ModelInput(m_model.GetExtension()).Get(ToUTF8(curName), modelInput);
			if (msg)
			{
				msg = m_model.VerifyModelInput(modelInput);
				if (!msg)
				{
					CString tmp;
					AfxFormatString1(tmp, IDS_MODELINPUTINALID, curName);
					msg.ajoute(ToUTF8(tmp));
				}
			}
			else
			{
				CString tmp;
				AfxFormatString1(tmp, IDS_MODELINPUTINALID, curName);
				msg.ajoute(ToUTF8(tmp));

				m_model.GetDefaultParameter(modelInput);
			}
		}


		return msg;
	}

	ERMsg CModelInputListBox::SaveModelInput(const CString& modelInputName, bool askConfirm)
	{
		ASSERT(!modelInputName.IsEmpty() && modelInputName != STRDEFAULT);

		ERMsg msg;

		CModelInput modelInput;
		m_pModelInputDlg->GetModelInput(modelInput);

		bool bModified = modelInput != m_lastModelInput;

		if (bModified)
		{
			bool bSave = true;
			if (askConfirm)
			{
				CString sOutMessage;
				AfxFormatString1(sOutMessage, IDS_SAVE_MODELINPUT, modelInputName);
				bSave = MessageBox(sOutMessage, AfxGetAppName(), MB_YESNO) == IDYES;
			}

			if (bSave)
			{
				msg = GetFM().ModelInput(m_model.GetExtension()).Set(ToUTF8(modelInputName), modelInput);
			}
		}


		return msg;
	}


	//****************************************************************************************************************************

	CWGInputManagerDlg::CWGInputManagerDlg(CWnd* pParent) :
		CDialog(CWGInputManagerDlg::IDD, pParent)
	{
	}

	CWGInputManagerDlg::~CWGInputManagerDlg()
	{
	}


	void CWGInputManagerDlg::DoDataExchange(CDataExchange* pDX)
	{
		m_fileListCtrl.m_model = m_model;

		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELIN_LIST, m_fileListCtrl);
		DDX_Control(pDX, IDC_MODELIN_FILEPATH, m_filePathCtrl);
	}


	BEGIN_MESSAGE_MAP(CWGInputManagerDlg, CDialog)
		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_MODELIN_LIST, OnSelChange)
		ON_NOTIFY(ON_BLB_NAMECHANGE, IDC_MODELIN_LIST, OnSelChange)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CWGInputManagerDlg msg handlers

	BOOL CWGInputManagerDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = option.GetProfilePoint(_T("WGInputManager"), CPoint(30, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		//select current tg input
		m_fileListCtrl.SelectString(m_WGInputName);

		UpdateCtrl();
		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CWGInputManagerDlg::UpdateCtrl()
	{
		BOOL bEnable = m_fileListCtrl.GetSelectedCount() == 1;

		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);

		int iItem = m_fileListCtrl.GetSelItem();
		string name = ToUTF8(m_fileListCtrl.GetItemText(iItem));

		string filePath = iItem > 0 ? GetFM().WGInput().GetFilePath(name) : GetFM().WGInput().GetLocalPath();
		m_filePathCtrl.SetWindowText(Convert(filePath));

	}

	void CWGInputManagerDlg::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	{
		UpdateCtrl();
	}

	void CWGInputManagerDlg::OnOK()
	{
		ERMsg msg;

		ASSERT(m_fileListCtrl.GetSelectedCount() == 1);

		m_WGInputName.Empty();
		if (!m_fileListCtrl.IsDefault())
		{
			m_WGInputName = m_fileListCtrl.GetWindowText();
			msg = m_fileListCtrl.SaveWGInput(m_WGInputName, false);

			if (!msg)
			{
				SYShowMessage(msg, this);
				return;
			}
		}

		CDialog::OnOK();
	}

	//void CWGInputManagerDlg::OnCancel()
	//{
	//	//Do nothing
	//}

	BOOL CWGInputManagerDlg::DestroyWindow()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("WGInputManager"), pt);

		return CDialog::DestroyWindow();
	}


	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CWGInputListBox, CBioSIMListBox)
	END_MESSAGE_MAP()


	CWGInputListBox::CWGInputListBox() :CBioSIMListBox(true)
	{
		m_pWGInputDlg = new CWGInputDlg(this);
		m_lastSelection = -1;
	}

	CWGInputListBox::~CWGInputListBox()
	{
		ASSERT(m_pWGInputDlg);

		if (m_pWGInputDlg->m_hWnd != NULL)
			m_pWGInputDlg->DestroyWindow();

		delete m_pWGInputDlg;
		m_pWGInputDlg = NULL;
	}

	CWnd* CWGInputListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_SET_DEFAULT);
		OnInitList();

		m_pWGInputDlg->Create(m_model, this);

		return m_pWndList;
	}


	void CWGInputListBox::OnAfterAddItem(int iItem)
	{
		string TGName = ToUTF8(GetItemText(iItem));

		CWGInput WGInput;
		WGInput.LoadDefaultParameter();

		ERMsg msg = GetFM().WGInput().Set(TGName, WGInput);
		if (msg)
		{
			m_pWGInputDlg->SetWGInput(WGInput);
			m_pWGInputDlg->UpdateCtrl(iItem > 0);
			m_lastSelection = iItem;
			m_lastWGInput = WGInput;
		}
		else
		{
			RemoveItem(iItem);
			SYShowMessage(msg, this);
		}
	}

	BOOL CWGInputListBox::OnBeforeCopyItem(int iItem, CString newName)
	{
		BOOL bRep = CBioSIMListBox::OnBeforeCopyItem(iItem, newName);
		if (bRep)
		{
			//m_lastSelection = iItem;//By RSA 2019-01-24
			CString TGName = newName;

			CWGInput WGInput;
			LoadWGInput(TGName, WGInput);
			m_pWGInputDlg->SetWGInput(WGInput);
			m_pWGInputDlg->UpdateCtrl(m_lastSelection > 0);
			m_lastWGInput = WGInput;
		}

		return bRep;
	}

	void CWGInputListBox::OnSetAsDefault(int iItem)
	{
		if (iItem > 0)
		{
			CString TGName = GetItemText(iItem);

			if (!SaveWGInput(TGName, false))
				return;


			if (AfxMessageBox(IDS_DB_SET_AS_DEFAULT_CONFIRM, MB_YESNOCANCEL) == IDYES)
			{
				CWGInput WGInput;
				VERIFY(LoadWGInput(TGName, WGInput));
				WGInput.SetAsDefaultParameter();
			}
		}
	}


	void CWGInputListBox::OnSelectionChanged()
	{
		ERMsg msg;

		//event if they have error, we don't take care of
		if (m_lastSelection > 0 && !GetItemText(m_lastSelection).IsEmpty())
		{
			SaveWGInput(GetItemText(m_lastSelection));
		}

		m_lastSelection = GetSelItem();
		CString TGName = GetItemText(m_lastSelection);
		

		CWGInput WGInput;
		msg = LoadWGInput(TGName, WGInput);
		m_pWGInputDlg->SetWGInput(WGInput);
		m_pWGInputDlg->UpdateCtrl(m_lastSelection>0);

		m_lastWGInput = WGInput;

		if (!msg)
			SYShowMessage(msg, this);


		//call parent
		CBioSIMListBox::OnSelectionChanged();
	}

	ERMsg CWGInputListBox::LoadWGInput(const CString& curName, CWGInput& WGInput)
	{
		ERMsg msg;
		if (curName.IsEmpty() || curName == STRDEFAULT)
		{
			WGInput.LoadDefaultParameter();
		}
		else
		{
			msg = GetFM().WGInput().Get(ToUTF8(curName), WGInput);
			if (msg)
			{
				msg = WGInput.IsValid();
				if (!msg)
				{
					CString tmp;
					AfxFormatString1(tmp, IDS_MODELINPUTINALID, curName);
					msg.ajoute(ToUTF8(tmp));
				}
			}
			else
			{
				CString tmp;
				AfxFormatString1(tmp, IDS_MODELINPUTINALID, curName);
				msg.ajoute(ToUTF8(tmp));

				WGInput.LoadDefaultParameter();
			}

		}

		return msg;
	}

	ERMsg CWGInputListBox::SaveWGInput(const CString& TGName, bool askConfirm)
	{
		ASSERT(!TGName.IsEmpty() && TGName != STRDEFAULT);

		ERMsg msg;

		CWGInput WGInput;
		m_pWGInputDlg->GetWGInput(WGInput);
		
		msg = WGInput.IsValid();
		if (msg)
		{
			bool bModified = WGInput != m_lastWGInput;

			if (bModified)
			{
				bool bSave = true;
				if (askConfirm)
				{
					CString sOutMessage;
					AfxFormatString1(sOutMessage, IDS_SAVE_MODELINPUT, TGName);
					bSave = MessageBox(sOutMessage, AfxGetAppName(), MB_YESNO) == IDYES;
				}

				if (bSave)
				{
					msg = GetFM().WGInput().Set(ToUTF8(TGName), WGInput);
				}
			}
		}

		return msg;
	}


	//********************************************************************************
	// CParametersVariationsListBox

	BEGIN_MESSAGE_MAP(CParametersVariationsListBox, CBioSIMListBox)
		//ON_WM_PAINT()
	END_MESSAGE_MAP()



	CParametersVariationsListBox::CParametersVariationsListBox(const CModel& model, bool bAddDefault, bool bShowVariationType) :
		CBioSIMListBox(bAddDefault),
		m_model(model)
	{
		m_pPVDDlg = new CParametersVariationsDlg(this, bShowVariationType);
		m_lastSelection = -1;
	}

	CParametersVariationsListBox::~CParametersVariationsListBox()
	{
		ASSERT(m_pPVDDlg);

		if (m_pPVDDlg->m_hWnd != NULL)
			m_pPVDDlg->DestroyWindow();

		delete m_pPVDDlg;
		m_pPVDDlg = NULL;
	}

	CWnd* CParametersVariationsListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_SET_DEFAULT);
		OnInitList();

		m_pPVDDlg->m_model = m_model;
		m_pPVDDlg->m_parametersDefinition = m_model.GetInputDefinition(true);
		m_pPVDDlg->m_parametersVariations = m_pPVDDlg->m_parametersDefinition.GetParametersVariations();
		m_pPVDDlg->Create(CParametersVariationsDlg::IDD, this);


		return m_pWndList;
	}


	void CParametersVariationsListBox::OnAfterAddItem(int iItem)
	{
		string name = ToUTF8(GetItemText(iItem));

		const CModelInputParameterDefVector& varDef = m_model.GetInputDefinition(true);
		CParametersVariationsDefinition PVD = varDef.GetParametersVariations();
		ERMsg msg = GetFM().PVD(m_model.GetExtension()).Set(name, PVD);

		if (msg)
		{
			//m_pPVDDlg->m_parametersDefinition = varDef;
			m_pPVDDlg->m_parametersVariations = PVD;
			m_pPVDDlg->SelectParameters();
			//m_pPVDDlg->EnableWindow(!name.empty() && !IsDefault());
			m_pPVDDlg->Enable(!name.empty() && !IsDefault());


			m_lastSelection = iItem;
			m_lastPVD = PVD;
		}
		else
		{
			RemoveItem(iItem);
			SYShowMessage(msg, this);
		}
	}

	BOOL CParametersVariationsListBox::OnBeforeCopyItem(int iItem, CString newName)
	{
		BOOL bRep = CBioSIMListBox::OnBeforeCopyItem(iItem, newName);
		if (bRep)
		{
			//m_lastSelection = iItem;//By RSA 2019-01-24
			LoadPVDInput(newName, m_pPVDDlg->m_parametersVariations);
			m_lastPVD = m_pPVDDlg->m_parametersVariations;
			m_pPVDDlg->SelectParameters();
		}

		return bRep;
	}



	void CParametersVariationsListBox::OnSelectionChanged()
	{
		ERMsg msg;

		

		//event if they have error, we don't take care of
		CString name = GetItemText(m_lastSelection);
		


		if (!name.IsEmpty() && m_lastSelection>0)
			SavePVDInput(name, true);

		m_lastSelection = GetSelItem();
		CString PVDName = GetItemText(m_lastSelection);
		m_pPVDDlg->Enable(!PVDName.IsEmpty() && !IsDefault());


		msg = LoadPVDInput(PVDName, m_pPVDDlg->m_parametersVariations);
		m_pPVDDlg->SelectParameters();
		m_lastPVD = m_pPVDDlg->m_parametersVariations;


		if (!msg)
			SYShowMessage(msg, this);

		//m_pPVDDlg->EnableWindow(!PVDName.IsEmpty() && !IsDefault());
		

		//call parent
		CBioSIMListBox::OnSelectionChanged();
	}

	ERMsg CParametersVariationsListBox::LoadPVDInput(const CString& curName, CParametersVariationsDefinition& PVD)
	{
		ERMsg msg;

		const CModelInputParameterDefVector& varDef = m_model.GetInputDefinition(true);
		if (!curName.IsEmpty() && curName != STRDEFAULT)
			msg = GetFM().PVD(m_model.GetExtension()).Get(ToUTF8(curName), PVD);
		

		
		if (!msg || PVD.size() != varDef.size())
			PVD = varDef.GetParametersVariations();
		

		return msg;
	}

	ERMsg CParametersVariationsListBox::SavePVDInput(const CString& PVDName, bool askConfirm)
	{
		ASSERT(!PVDName.IsEmpty() && PVDName != STRDEFAULT);


		ERMsg msg;

		bool bModified = m_pPVDDlg->m_parametersVariations != m_lastPVD;

		if (bModified)
		{
			bool bSave = true;
			if (askConfirm)
			{
				CString sOutMessage;
				AfxFormatString1(sOutMessage, IDS_SAVE_MODELINPUT, PVDName);
				bSave = MessageBox(sOutMessage, AfxGetAppName(), MB_YESNO) == IDYES;
			}

			if (bSave)
			{
				msg = GetFM().PVD(m_model.GetExtension()).Set(ToUTF8(PVDName), m_pPVDDlg->m_parametersVariations);
			}
		}

		return msg;
	}


	//****************************************************************************************************************************

	CParametersVariationsManagerDlg::CParametersVariationsManagerDlg(bool bAddDefault, bool bShowVariationType, CWnd* pParent) :
		CDialog(CParametersVariationsManagerDlg::IDD, pParent),
		m_fileListCtrl(m_model, bAddDefault, bShowVariationType)
	{
	}

	CParametersVariationsManagerDlg::~CParametersVariationsManagerDlg()
	{
	}


	void CParametersVariationsManagerDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELIN_LIST, m_fileListCtrl);
		DDX_Control(pDX, IDC_MODELIN_FILEPATH, m_filePathCtrl);
	}


	BEGIN_MESSAGE_MAP(CParametersVariationsManagerDlg, CDialog)
		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_MODELIN_LIST, OnSelChange)
		ON_NOTIFY(ON_BLB_NAMECHANGE, IDC_MODELIN_LIST, OnSelChange)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CParametersVariationsManagerDlg msg handlers

	BOOL CParametersVariationsManagerDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = option.GetProfilePoint(_T("PVDManager"), CPoint(30, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		//select current tg input
		m_fileListCtrl.SelectString(m_parametersVariationsName);
		UpdateCtrl();


		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CParametersVariationsManagerDlg::UpdateCtrl()
	{
		BOOL bEnable = m_fileListCtrl.GetSelectedCount() == 1;

		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);

		int iItem = m_fileListCtrl.GetSelItem();
		string name = ToUTF8(m_fileListCtrl.GetItemText(iItem));

		string filePath = m_fileListCtrl.IsDefault() ? GetFM().PVD(m_model.GetExtension()).GetLocalPath() : GetFM().PVD(m_model.GetExtension()).GetFilePath(name);
		m_filePathCtrl.SetWindowText(Convert(filePath));

	}

	void CParametersVariationsManagerDlg::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	{
		UpdateCtrl();
	}

	void CParametersVariationsManagerDlg::OnOK()
	{
		ERMsg msg;

		ASSERT(m_fileListCtrl.GetSelectedCount() == 1);

		m_parametersVariationsName.Empty();
		if (!m_fileListCtrl.IsDefault())
		{
			m_parametersVariationsName = m_fileListCtrl.GetWindowText();
			msg = m_fileListCtrl.SavePVDInput(m_parametersVariationsName, false);

			if (!msg)
			{
				SYShowMessage(msg, this);
				return;
			}
		}

		CDialog::OnOK();
	}


	//void CParametersVariationsManagerDlg::OnCancel()
	//{
	//	//Do nothing
	//}

	BOOL CParametersVariationsManagerDlg::DestroyWindow()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("PVDManager"), pt);

		return CDialog::DestroyWindow();
	}

}