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

#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h" 
#include "UI/WVariablesEdit.h"
#include "UI/ModelInputDlg.h"
#include "UI/ModelPages.h"


#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace WBSF
{


	//*************************************************************************************
	// CModelGeneralPage property page

	BEGIN_MESSAGE_MAP(CModelGeneralPage, CMFCPropertyPage)
		ON_EN_CHANGE(IDC_MODELS_DLLNAME, &CModelGeneralPage::OnDLLNameChange)
	END_MESSAGE_MAP()


	CModelGeneralPage::CModelGeneralPage(CModel& model) :
		CMFCPropertyPage(CModelGeneralPage::IDD),
		m_model(model)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelGeneralPage::~CModelGeneralPage()
	{
	}

	void CModelGeneralPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELS_DLLNAME, m_DLLNameCtrl);
		DDX_Control(pDX, IDC_MODELS_DLLVERSION, m_DLLVersionCtrl);
		DDX_Text(pDX, IDC_MODELS_NAME, m_model.m_title);
		DDX_Text(pDX, IDC_MODELS_VERSION, m_model.m_version);
		DDX_Text(pDX, IDC_MODELS_DLLNAME, m_model.m_DLLName);
		DDX_Text(pDX, IDC_MODELS_DOCUMENTATION_FILE, m_model.m_helpFileName);
		DDX_Text(pDX, IDC_MODELS_DESCRIPTION, m_model.m_description);
		DDX_CBIndex(pDX, IDC_MODELS_BEHAVIOUR, m_model.m_behaviour);


		m_DLLNameCtrl.Init(Convert(m_model.GetPath()));

		UpdateDLLVersion();
	}


	void CModelGeneralPage::UpdateDLLVersion()
	{
		CString DLLName;
		m_DLLNameCtrl.GetWindowText(DLLName);
		if (!DLLName.IsEmpty())
		{
			string filePath = m_model.GetDLLFilePath(ToUTF8(DLLName));
			string DLLVersion = m_model.GetDLLVersion(filePath);

			m_DLLVersionCtrl.SetWindowText(Convert(DLLVersion));
		}
	}


	void CModelGeneralPage::OnDLLNameChange()
	{
		UpdateDLLVersion();
	}




	//*************************************************************************************
	// CModelWGInputPage property page
	BEGIN_MESSAGE_MAP(CModelWGInputPage, CMFCPropertyPage)
		ON_CBN_SELCHANGE(IDC_MODELS_TRANSFER_FILE_VERSION, &CModelWGInputPage::OnTransferFileVersionChange)
	END_MESSAGE_MAP()


	CModelWGInputPage::CModelWGInputPage(CModel& model) :
		CMFCPropertyPage(CModelWGInputPage::IDD),
		m_model(model)
	{

		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelWGInputPage::~CModelWGInputPage()
	{}

	void CModelWGInputPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);


		DDX_Control(pDX, IDC_MODELS_VARIABLES, m_variablesCtrl);
		DDX_WVariables(pDX, IDC_MODELS_VARIABLES, m_model.m_variables);
		DDX_Text(pDX, IDC_MODELS_NBYEAR_MIN, m_model.m_nbYearMin);
		DDX_Text(pDX, IDC_MODELS_NBYEAR_MAX, m_model.m_nbYearMax);
		DDX_CBIndex(pDX, IDC_MODELS_TRANSFER_FILE_VERSION, m_model.m_transferFileVersion);
		DDX_Check(pDX, IDC_MODELS_THREAD_SAFE, m_model.m_bThreadSafe);

		//update combobox
		OnTransferFileVersionChange();
	}



	void CModelWGInputPage::OnTransferFileVersionChange()
	{
		int i = ((CComboBox*)GetDlgItem(IDC_MODELS_TRANSFER_FILE_VERSION))->GetCurSel();
		GetDlgItem(IDC_MODELS_IO_FILE_VERSION)->EnableWindow(i != CModel::VERSION_STREAM);
	}

	//****************************************************************************************
	//CModelSSIPage

	BEGIN_MESSAGE_MAP(CModelSSIPage, CMFCPropertyPage)
		//ON_CBN_SELCHANGE(IDC_MODELS_TRANSFER_FILE_VERSION, &CModelSSIPage::OnTransferFileVersionChange)
	END_MESSAGE_MAP()


	CModelSSIPage::CModelSSIPage(CModel& model) :
		CMFCPropertyPage(CModelSSIPage::IDD),
		m_model(model)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelSSIPage::~CModelSSIPage()
	{}

	void CModelSSIPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_MODELS_SSI, m_SSICtrl);
		//DDX_Text(pDX, IDC_MODELS_SSI, m_model.m_SSI);

		if (pDX->m_bSaveAndValidate)
		{
			for (int i = 0; i < m_SSICtrl.GetCount(); i++)
			{
				if (!m_model.m_SSI.empty())
					m_model.m_SSI += "¦";

				m_model.m_SSI += UtilWin::ToUTF8(m_SSICtrl.GetItemText(i));
			}
		}
		else
		{
			StringVector list(m_model.m_SSI, "¦");
			for (size_t i = 0; i < list.size(); i++)
			{
				m_SSICtrl.AddItem(CString(list[i].c_str()));
			}
		}
	}



	//*************************************************************************************
	// CModelInputPage property page

	CRectProp::CRectProp(const CString& strGroupName, DWORD_PTR dwData, BOOL bIsValueList) :
		CCFLPropertyGridProperty(strGroupName, dwData, bIsValueList)
	{
	}

	BOOL CRectProp::OnUpdateValue()
	{
		ASSERT_VALID(this);
		ASSERT_VALID(m_pWndInPlace);
		ASSERT_VALID(m_pWndList);
		ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));

		CString strText;
		m_pWndInPlace->GetWindowText(strText);

		BOOL bIsChanged = FormatProperty() != strText;

		if (bIsChanged)
		{
			return CMFCPropertyGridProperty::OnUpdateValue();
		}

		return TRUE;
	}

	void CRectProp::SetValue(const COleVariant& varValue)
	{
		ASSERT(GetSubItemsCount() == 4);

		CString str = varValue;
		CRect rect = CRectFromCString(str);

		GetSubItem(0)->SetValue((long)rect.top);
		GetSubItem(1)->SetValue((long)rect.left);
		GetSubItem(2)->SetValue((long)rect.Height());
		GetSubItem(3)->SetValue((long)rect.Width());
	}

	const COleVariant& CRectProp::GetValue() const
	{
		ASSERT(GetSubItemsCount() == 4);

		CRect rect;
		rect.top = GetSubItem(0)->GetValue().lVal;
		rect.left = GetSubItem(1)->GetValue().lVal;
		rect.bottom = rect.top + GetSubItem(2)->GetValue().lVal;
		rect.right = rect.left + GetSubItem(3)->GetValue().lVal;

		COleVariant& varValue = const_cast<COleVariant&> (m_varValue);
		CString str = UtilWin::ToCString(rect);
		varValue = str;

		return m_varValue;
	}

	////////////////////////////////////////////////////////////////////////////////
	// CBoundedNumberSubProp class

	CBoundedNumberSubProp::CBoundedNumberSubProp(const CString& strName, const COleVariant& varValue, /*int nMinValue, int nMaxValue,*/ LPCTSTR lpszDescr) :
		CMFCPropertyGridProperty(strName, varValue, lpszDescr)
	{
	}

	BOOL CBoundedNumberSubProp::OnUpdateValue()
	{
		ASSERT_VALID(this);
		ASSERT_VALID(m_pWndInPlace);
		ASSERT_VALID(m_pWndList);
		ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));

		BOOL bRet = TRUE;
		CString strText;
		m_pWndInPlace->GetWindowText(strText);

		BOOL bIsChanged = FormatProperty() != strText;
		if (bIsChanged)
		{
			int nItem = _ttoi(strText);
			bRet = CMFCPropertyGridProperty::OnUpdateValue();

			if (m_pParent != NULL)
			{
				m_pWndList->OnPropertyChanged(m_pParent);
			}
		}

		return bRet;
	}

	//*****************************************************************************
	BEGIN_MESSAGE_MAP(CModelInputParameterCtrl, CPropertiesListBox)
		ON_WM_DESTROY()
	END_MESSAGE_MAP()

	CModelInputParameterCtrl::CModelInputParameterCtrl() :
		CPropertiesListBox(150),
		m_lastType(CModelInputParameterDef::kMVReal)
	{
		m_pModelInput = NULL;
	}

	CWnd* CModelInputParameterCtrl::OnCreateProperties()
	{
		if (!CPropertiesListBox::OnCreateProperties())
			return FALSE;

		// create the form
		CAppOption option;

		CRect parentRect;
		if (GetParent())
			GetParent()->GetWindowRect(parentRect);

		CRect rect = option.GetProfileRect(_T("ModelInputFormPosition"), CRect(parentRect.right, parentRect.top, parentRect.right + 250, parentRect.top + 100));
		UtilWin::EnsureRectangleOnDisplay(rect);
		VERIFY(m_form.Create(rect, IDD_MODELS_FORM, this));


		ENSURE_VALID(m_pWndProperties);

		CStringArrayEx INPUT_TYPE_NAME(IDS_SIM_INPUT_PARAM_TYPE);
		CStringArrayEx title(IDS_SIM_INPUT_PARAM_TITLE);
		CStringArrayEx description(IDS_SIM_INPUT_PARAM_DESCRIPTION);
		CStringArrayEx rectangleTitle(IDS_SIM_INPUT_RECTANGLE);
		CStringArrayEx rectangleDescription(IDS_SIM_INPUT_RECTANGLE_DESC);
		

		for (int i = 0; i < CModelInputParameterDef::NB_MEMBER; i++)
		{

			CCFLPropertyGridProperty* pProp = new CCFLPropertyGridProperty(title[i], _T(""), description[i]);
			if (i == CModelInputParameterDef::NAME)
			{
				pProp->AllowEdit(FALSE);
			}
			else if (i == CModelInputParameterDef::TYPE)
			{
				ASSERT(CModelInputParameterDef::kMVNbType == INPUT_TYPE_NAME.GetSize());
				for (int j = 0; j < CModelInputParameterDef::kMVNbType; j++)
				{
					pProp->AddOption(INPUT_TYPE_NAME[j]);
				}

				pProp->AllowEdit(FALSE);
			}
			else if (i == CModelInputParameterDef::RECT)
			{
				delete pProp;
				pProp = (CCFLPropertyGridProperty*)new CRectProp(title[i], 0, TRUE);
				pProp->SetDescription(description[i]);
				pProp->AllowEdit(false);
				for (int j = 0; j < 4; j++)
				{
					pProp->AddSubItem(new CBoundedNumberSubProp(rectangleTitle[j], (COleVariant)0l, rectangleDescription[j]));
					pProp->GetSubItem(j)->AllowEdit(false);
				}
			}
			

			m_pWndProperties->AddProperty(pProp);
		}



		return m_pWndProperties;
	}

	BOOL CModelInputParameterCtrl::OnBeforeRemoveItem(int iItem)
	{
		UINT pos = (int)m_pWndList->GetItemData(iItem);

		m_form.DeleteItem(pos);
		m_data.erase(m_data.begin() + pos);


		return CPropertiesListBox::OnBeforeRemoveItem(iItem);
	}

	void CModelInputParameterCtrl::OnAfterAddItem(int iItem)
	{
		//	ASSERT( iItem == m_data.GetSize() );
		ASSERT(iItem == (int)m_data.size());

		CRect formRect = m_form.GetRect();

		string name = ToUTF8(m_pWndList->GetItemText(iItem));

		ASSERT(CModelInputParameterDef::TYPE < m_pWndProperties->GetPropertyCount());
		CCFLPropertyGridProperty* pProp = m_pWndProperties->GetProperty(CModelInputParameterDef::TYPE);


		int x = 10;
		int y = 10 + CModelInputParameterDef::DEFAULT_HEIGHT*iItem;
		if (y > formRect.Height())
			y = formRect.Height() - 10;

		CRect rect(x, y, x + CModelInputParameterDef::DEFAULT_WIDTH, y + CModelInputParameterDef::DEFAULT_HEIGHT);
		CModelInputParameterDefPtr pParam(new CModelInputParameterDef(name, name, m_lastType, "", "", rect));
		//m_data.Add(pParam);
		size_t pos = m_data.size();
		m_data.push_back(std::move(pParam));
		m_form.AddItem(*m_data[pos]);


		//	ASSERT( m_data.GetSize() == m_pWndList->GetCount() );
		ASSERT((int)m_data.size() == m_pWndList->GetCount());
		CPropertiesListBox::OnAfterAddItem(iItem);

		//Update the selection because we don't receve it from parent

	}

	void CModelInputParameterCtrl::OnAfterRenameItem(int iItem)
	{
		//ASSERT( m_data.GetSize() == m_pWndList->GetCount() );
		ASSERT((int)m_data.size() == m_pWndList->GetCount());

		int pos = (int)m_pWndList->GetItemData(iItem);
		m_data[pos]->SetName(ToUTF8(m_pWndList->GetItemText(iItem)));

		CPropertiesListBox::OnAfterRenameItem(iItem);
	}

	int CModelInputParameterCtrl::FindName(const CString& name)
	{
		int index = -1;
		for (size_t i = 0; i < m_data.size(); i++)
		{
			if (name == m_data[i]->GetName().c_str())
			{
				index = (int)i;
				break;
			}
		}

		return index;
	}

	void CModelInputParameterCtrl::OnItemActivation(const CString& name)
	{
		int pos = FindName(name);

		ASSERT(pos >= 0 && pos < m_pWndList->GetCount());
		if (pos != m_pWndList->GetSelItem())
		{
			m_pWndList->UnselectAll();
			m_pWndList->SelectItem(pos);
		}
	}

	void CModelInputParameterCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
	{
		CModelInputParameterCtrl& me = const_cast<CModelInputParameterCtrl&>(*this);

		int pos = m_pWndList->GetSelItem();
		ASSERT(pos >= 0 && pos < m_pWndList->GetCount());

		int i = (int)pProp->GetData();
		if (i == CModelInputParameterDef::TYPE)
		{
			int index = ((const CCFLPropertyGridProperty*)pProp)->GetCurSel();
			me.m_data[pos]->SetMember(i, WBSF::ToString(index));
			me.m_lastType = index;
		}
		else
		{
			CString tmp = pProp->GetValue();
			m_data[pos]->SetMember(i, ToUTF8(tmp));
		}

		me.m_form.UpdateItem(pos);

	}


	void CModelInputParameterCtrl::OnGetDataFromProperties(int iItem)
	{
		ENSURE(m_lastSel < (int)m_data.size());
		ENSURE(m_pWndProperties);

		int pos = (int)m_pWndList->GetItemData(iItem);
		for (int i = 0; i < m_pWndProperties->GetPropertyCount(); i++)
		{
			CCFLPropertyGridProperty* pProp = m_pWndProperties->GetProperty(i);

			if (i == CModelInputParameterDef::TYPE)
			{
				int index = pProp->GetCurSel();
				m_data[pos]->SetMember(i, WBSF::ToString(index));
			}
			else
			{
				CString tmp = pProp->GetValue();
				m_data[pos]->SetMember(i, ToUTF8(tmp));
			}
		}

	}

	void CModelInputParameterCtrl::OnSetDataToProperties(int iItem)
	{
		ASSERT(iItem >= -1 && iItem < m_pWndList->GetCount());

		m_pWndProperties->EnableWindow(iItem >= 0);
		int pos = -1;
		if (iItem >= 0)
			pos = (int)m_pWndList->GetItemData(iItem);

		for (int i = 0; i < m_pWndProperties->GetPropertyCount(); i++)
		{
			CCFLPropertyGridProperty* pProp = m_pWndProperties->GetProperty(i);

			if (iItem >= 0)
			{
				if (i == CModelInputParameterDef::TYPE)
				{
					int index = WBSF::ToInt(m_data[pos]->GetMember(i));
					pProp->SetCurSel(index);
				}
				else
				{
					pProp->SetValue(Convert(m_data[pos]->GetMember(i)));
				}
			}
			else
			{
				pProp->SetValue(_T(""));
			}
		}
	}

	void CModelInputParameterCtrl::GetData(CModelInputParameterDefVector& data)
	{
		ASSERT(m_lastSel >= -1 && m_lastSel < (int)m_data.size());
		//Verify that we have the latest values
		if (m_lastSel != -1)
			OnGetDataFromProperties(m_lastSel);

		data.resize(m_pWndList->GetCount());
		for (int i = 0; i < m_pWndList->GetCount(); i++)
		{
			int pos = (int)m_pWndList->GetItemData(i);
			data[i] = *m_data[pos];

			data[i].CleanList();//clean list from cariage return
		}
	}

	void CModelInputParameterCtrl::SetData(const CModelInputParameterDefVector& data)
	{
		ASSERT(m_pWndList);

		//m_data.RemoveAll();
		m_data.clear();
		m_pWndList->DeleteAllItems();
		m_form.DeleteAllItem();
		m_lastSel = -1;

		for (int i = 0; i < data.size(); i++)
		{
			CModelInputParameterDefPtr pParam(new CModelInputParameterDef(data[i]));
			pParam->CleanList();//clean list from carriage return

			size_t pos = m_data.size();
			m_data.push_back(std::move(pParam));
			m_pWndList->AddItem(Convert(m_data[pos]->GetName()), i);
			m_form.AddItem(*m_data[pos]);

		}
	}

	void CModelInputParameterCtrl::OnSelectionChanged()
	{
		CPropertiesListBox::OnSelectionChanged();

		int sel = m_pWndList->GetSelItem();

		if (sel >= 0)
			m_form.SetCurSel(sel);
	}

	void CModelInputParameterCtrl::UpdateProperties(const CString& name)
	{
		int pos1 = FindName(name);
		int pos2 = m_pWndList->GetSelItem();

		ASSERT(pos1 >= 0 && pos1 < m_pWndList->GetCount());

		ASSERT(pos1 == pos2);
		if (pos1 == pos2)
		{
			OnSetDataToProperties(pos1);
		}
	}

	void CModelInputParameterCtrl::UpdateForm()
	{
		int sel = m_pWndList->GetSelItem();
		ASSERT(sel != -1);

		OnGetDataFromProperties(sel);
		m_form.UpdateItem(sel);
		m_form.Invalidate();
		m_form.UpdateWindow();

	}

	void CModelInputParameterCtrl::HideMember(int memberID)
	{
		CMFCPropertyGridProperty* pProp = m_pWndProperties->GetProperty(memberID);
		ASSERT(pProp);

		pProp->Show(FALSE);
	}

	void CModelInputParameterCtrl::ShowForm(bool bShow)
	{
		m_form.ShowWindow(bShow ? SW_SHOW : SW_HIDE);
		if (!bShow && m_pModelInput != NULL && m_pModelInput->m_hWnd)
			m_pModelInput->DestroyWindow();
	}


	void CModelInputParameterCtrl::OnDestroy()
	{
		if (m_pModelInput)
		{
			if (m_pModelInput->m_hWnd)
				m_pModelInput->DestroyWindow();

			delete m_pModelInput;
			m_pModelInput = NULL;
		}


		CRect rect = m_form.GetRect();

		CAppOption option;
		option.WriteProfileRect(_T("ModelInputFormPosition"), rect);

		CPropertiesListBox::OnDestroy();
	}

	void CModelInputParameterCtrl::TestForm()
	{
		//assert(false);//todo

		CModelInputParameterDefVector data;
		GetData(data);

		if (m_pModelInput == NULL)
			m_pModelInput = new CModelInputDlg;

		if (m_pModelInput->m_hWnd)
			m_pModelInput->DestroyWindow();


		CRect rect = m_form.GetRect();
		rect.MoveToXY(0, 0);
		m_pModelInput->Create(data, rect, _T(""), this, true);
	}



	CModelInputPage::CModelInputPage(CModel& model) :
		CMFCPropertyPage(CModelInputPage::IDD),
		m_model(model)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelInputPage::~CModelInputPage()
	{}

	void CModelInputPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_MODELS_INPUT_PARAMETERS, m_inputParamCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			m_inputParamCtrl.GetData(m_model.m_inputList);
			m_model.m_windowRect = m_inputParamCtrl.GetFormRect();
			//m_model.m_TM = CTM(m_outputTypeCtrl.GetCurSel(), m_outputModeCtrl.GetCurSel());
		}
		else
		{
			m_inputParamCtrl.SetData(m_model.m_inputList);
			m_inputParamCtrl.SetFormRect(m_model.m_windowRect);
		}

		DDX_Text(pDX, IDC_MODELS_EXTENSION, m_model.m_extension);
	}


	BEGIN_MESSAGE_MAP(CModelInputPage, CMFCPropertyPage)
		ON_BN_CLICKED(IDC_MODELS_TEST, OnTest)
	END_MESSAGE_MAP()


	BOOL CModelInputPage::OnSetActive()
	{
		m_inputParamCtrl.ShowForm(true);


		return CMFCPropertyPage::OnSetActive();
	}

	BOOL CModelInputPage::OnKillActive()
	{
		m_inputParamCtrl.ShowForm(false);

		return UpdateData();
	}


	void CModelInputPage::OnTest()
	{

		m_inputParamCtrl.TestForm();
	}

	//*************************************************************************************
	// CModelOutputPage property page

	class CTimeModePropertyGridProperty : public CStdGridProperty
	{
	public:

		//CTimeModePropertyGridProperty(const CString& strName, CTM TM = CTM(CTM::ATEMPORAL), LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0) :
		//	CMFCPropertyGridProperty(strName, _T(""), lpszDescr, dwData)
		
		CTimeModePropertyGridProperty(const std::string& name, CTM TM, const std::string& description, size_t no) :
			CStdGridProperty(name, WBSF::to_string(TM), description, no)
		{
			CStringArrayEx OPTIONS_VALUES(UtilWin::GetCString(IDS_STR_TM_TYPE));

			for (int i = 0; i < CTM::NB_REFERENCE; i++)
				AddOption(OPTIONS_VALUES[i]);

			if (TM.Type() < 0 || TM.Type() > CTM::NB_REFERENCE)
				TM = CTM(CTM::ATEMPORAL);

			m_bAllowEdit = false;
			SetOriginalValue(GetOptionText(TM.Type()));
		}

		CString GetOptionText(size_t index)
		{
			ASSERT(index < (size_t)m_lstOptions.GetSize());
			POSITION pos = m_lstOptions.FindIndex(index);
			return m_lstOptions.GetAt(pos);
		}

		CTM GetTM()const
		{
			CString str(CMFCPropertyGridProperty::GetValue());

			int index = 0;

			POSITION pos = m_lstOptions.Find(str);
			POSITION curPos = m_lstOptions.GetHeadPosition();
			while (pos != curPos)
			{
				m_lstOptions.GetNext(curPos);
				index++;
			}

			ASSERT(index >= 0 && index < CTM::NB_REFERENCE);

			return CTM(index);
		}

		void SetTM(CTM TM)
		{
			if (TM.Type() < 0 || TM.Type() > CTM::NB_REFERENCE)
				TM = CTM(CTM::ATEMPORAL);

			CMFCPropertyGridProperty::SetValue(GetOptionText(TM.Type()));
		}

		virtual const COleVariant& GetValue()const
		{
			CTM TM = GetTM();
			ASSERT(TM.IsValid());

			const_cast<CTimeModePropertyGridProperty*>(this)->m_TM = CString(WBSF::to_string(TM).c_str());

			return m_TM;
		}

		virtual void SetValue(const COleVariant& varValue)
		{
			CStringA str(varValue);

			CTM TM = WBSF::from_string<CTM>((LPCSTR)str);
			SetTM(TM);
		}

		//string GetString()const
		//{
		//	assert(false);
		//}

		//void SetString(const string& str)
		//{
		//	assert(false);
		//	//CString str(to_string(TM).c_str());
		//	//m_varValue = str;
		//}

		virtual std::string get_string(){ return std::string((LPCSTR)CStringA(CString(GetValue()))); }
		virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	protected:

		//CTM m_TM;
		COleVariant m_TM;
	};



	BEGIN_MESSAGE_MAP(CModelOutputVariableCtrl, CPropertiesListBox)
	END_MESSAGE_MAP()

	typedef CIndexProperty < IDS_STR_WEATHER_VARIABLES_TITLE, -1, true> CWeatherVariableProperty;

	CModelOutputVariableCtrl::CModelOutputVariableCtrl() :
		CPropertiesListBox(150)
	{}

	CWnd* CModelOutputVariableCtrl::OnCreateProperties()
	{
		if (!CPropertiesListBox::OnCreateProperties())
			return FALSE;

		ENSURE_VALID(m_pWndProperties);

		//CStringArrayEx OUTPUT_TYPE_NAME(IDS_STR_TM_TYPE);
		//CStringArrayEx EXTENDED_TYPE_NAME(IDS_STR_TM_MODE);
		//CStringArrayEx title(IDS_OUTPUT_VAR_TITLE);
		//CStringArrayEx description(IDS_OUTPUT_VAR_DESCRIPTION);
		StringVector title(IDS_OUTPUT_VAR_TITLE, "|;");
		StringVector description(IDS_OUTPUT_VAR_DESCRIPTION, "|;");
		


		for (size_t i = 0; i < CModelOutputVariableDef::NB_MEMBERS; i++)
		{
			CStdGridProperty* pProp = NULL;
			switch (i)
			{
			case CModelOutputVariableDef::NAME:
			case CModelOutputVariableDef::TITLE:
			case CModelOutputVariableDef::UNITS:
			case CModelOutputVariableDef::DESCRIPTION:
			case CModelOutputVariableDef::PRECISION:
			case CModelOutputVariableDef::EQUATION:		pProp = new CStdGridProperty(title[i], "", description[i], i); break;
			case CModelOutputVariableDef::TIME_MODE:	pProp = new CTimeModePropertyGridProperty(title[i], CTM(CTM::ATEMPORAL), description[i], i); break;
			case CModelOutputVariableDef::CLIMATIC_VARIABLE: pProp = new CWeatherVariableProperty(title[i], 0, description[i], i); break;
			default: ASSERT(false);
			}
			
			if (i == CModelOutputVariableDef::NAME)
				pProp->AllowEdit(FALSE);

			m_pWndProperties->AddProperty(pProp);
		}

		m_pWndProperties->SetLeftColumnWidth(80);


		return m_pWndProperties;
	}

	BOOL CModelOutputVariableCtrl::OnBeforeRemoveItem(int iItem)
	{
		UINT pos = (int)m_pWndList->GetItemData(iItem);
		m_data.erase(m_data.begin() + pos);

		return CPropertiesListBox::OnBeforeRemoveItem(iItem);
	}

	void CModelOutputVariableCtrl::OnAfterAddItem(int iItem)
	{
		ASSERT(iItem == m_data.size());

		string name = ToUTF8(m_pWndList->GetItemText(iItem));
		m_data.push_back(CModelOutputVariableDef(name, name, "", ""));

		ASSERT(m_data.size() == m_pWndList->GetCount());
		CPropertiesListBox::OnAfterAddItem(iItem);

		//Update the selection because we don't receive it from parent

	}

	void CModelOutputVariableCtrl::OnAfterRenameItem(int iItem)
	{
		ASSERT(m_data.size() == m_pWndList->GetCount());

		int pos = (int)m_pWndList->GetItemData(iItem);
		m_data[pos].m_name = ToUTF8(m_pWndList->GetItemText(iItem));

		CPropertiesListBox::OnAfterRenameItem(iItem);
	}

	void CModelOutputVariableCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		int iItem = m_pWndList->GetSelItem();
		ASSERT(iItem >= 0 && iItem < m_pWndList->GetCount());
		size_t pos = (size_t)m_pWndList->GetItemData(iItem);
		ASSERT(pos < m_data.size());

		if (pos < m_data.size())
		{
			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
			size_t member = (size_t)pProp->GetData();
			ASSERT(member < CModelOutputVariableDef::GetNbMembers());
			string str = pProp->get_string();
			CString test1 = pProp->GetValue();
			CString test2 = pProp->FormatProperty();

			const_cast<CModelOutputVariableCtrl*>(this)->m_data[pos].SetMember(member, str);
		}
	}

	void CModelOutputVariableCtrl::OnGetDataFromProperties(int iItem)
	{
		ENSURE(m_lastSel < m_data.size());
		ENSURE(m_pWndProperties);

		int pos = (int)m_pWndList->GetItemData(iItem);
		//for (int i = 0; i<m_pWndProperties->GetPropertyCount(); i++)
		ASSERT(m_pWndProperties->GetPropertyCount() == CModelOutputVariableDef::NB_MEMBERS);
		for (int i = 0; i < m_pWndProperties->GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pPropIn = m_pWndProperties->FindItemByData(i, 0);
			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
			ENSURE(pProp);

			size_t member = (size_t)pProp->GetData();
			if (member < m_data.size())
			{
				//CStringA  tmp(pProp->GetValue());
				string str = pProp->get_string();
				m_data[pos].SetMember(member, str);
			}
		}

	}

	void CModelOutputVariableCtrl::OnSetDataToProperties(int iItem)
	{
		ASSERT(iItem >= -1 && iItem < m_pWndList->GetCount());

		m_pWndProperties->EnableWindow(iItem >= 0);
		int pos = -1;
		if (iItem >= 0)
			pos = (int)m_pWndList->GetItemData(iItem);

		//for (int i = 0; i<m_pWndProperties->GetPropertyCount(); i++)
		for (int i = 0; i < /*CModelOutputVariableDef::NB_MEMBERS*/m_pWndProperties->GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pPropIn = m_pWndProperties->FindItemByData(i, 0);
			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
			ASSERT(pProp);

			if (iItem >= 0)
			{
				//int member = pProp->GetData();
				ASSERT(pos >= 0 && pos < (int)m_data.size());
				pProp->set_string(m_data[pos].GetMember(i));
				//pProp->SetValue(CString(m_data[pos].GetMember(i).c_str()));
			}
			else
			{
				pProp->set_string("");
				//pProp->SetValue(_T(""));
			}
		}
	}

	void CModelOutputVariableCtrl::GetData(CModelOutputVariableDefVector& data)
	{
		ASSERT(data.empty() || (m_lastSel >= -1 && m_lastSel < m_data.size()));
		//Verify that we have the latest values
		if (m_lastSel != -1)
			OnGetDataFromProperties(m_lastSel);

		data.resize(m_pWndList->GetCount());
		for (int i = 0; i < m_pWndList->GetCount(); i++)
		{
			int pos = (int)m_pWndList->GetItemData(i);
			data[i] = m_data[pos];
		}
	}

	void CModelOutputVariableCtrl::SetData(const CModelOutputVariableDefVector& data)
	{
		ASSERT(m_pWndList);

		m_pWndList->DeleteAllItems();
		m_lastSel = -1;

		m_data = data;
		for (int i = 0; i < m_data.size(); i++)
		{
			m_pWndList->AddItem(Convert(m_data[i].m_name), i);
		}


	}

	void CModelOutputVariableCtrl::Add(const CModelOutputVariableDef& elem)
	{
		int pos = (int)m_data.size();
		m_data.push_back(elem);
		m_pWndList->AddItem(Convert(m_data[pos].m_name), pos);
	}

	void CModelOutputVariableCtrl::HideMember(int memberID)
	{
		CMFCPropertyGridProperty* pProp = m_pWndProperties->FindItemByData(memberID, 0);
		//CMFCPropertyGridProperty* pProp = m_pWndProperties->GetProperty(memberID);
		ENSURE(pProp);

		pProp->Show(FALSE);
	}

	//*****************************************************************************

	CModelOutputPage::CModelOutputPage(CModel& model) :
		CMFCPropertyPage(CModelOutputPage::IDD),
		m_model(model)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelOutputPage::~CModelOutputPage()
	{}

	void CModelOutputPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELS_OUTPUT_TYPE, m_outputTypeCtrl);
		DDX_Control(pDX, IDC_MODELS_OUTPUT_MODE, m_outputModeCtrl);
		DDX_Control(pDX, IDC_MODELS_OUTPUT_VAR, m_outputVarCtrl);


		DDX_Text(pDX, IDC_MODELS_MISSING_VALUE, m_model.m_missValue);

		if (pDX->m_bSaveAndValidate)
		{
			m_outputVarCtrl.GetData(m_model.m_outputList);
			m_model.m_outputTM = CTM(m_outputTypeCtrl.GetCurSel(), m_outputModeCtrl.GetCurSel());
		}
		else
		{
			//m_outputVarCtrl.HideMember(CModelOutputVariableDef::TIME_MODE);
			m_outputVarCtrl.HideMember(CModelOutputVariableDef::EQUATION);
			m_outputVarCtrl.HideMember(CModelOutputVariableDef::CLIMATIC_VARIABLE);
			m_outputVarCtrl.SetData(m_model.m_outputList);
			m_outputTypeCtrl.SetCurSel((int)m_model.m_outputTM.Type());
			m_outputModeCtrl.SetCurSel((int)m_model.m_outputTM.Mode());
		}

	}


	BEGIN_MESSAGE_MAP(CModelOutputPage, CMFCPropertyPage)
	END_MESSAGE_MAP()

	//*************************************************************************************
	// CModelCreditPage property page


	CModelCreditPage::CModelCreditPage(CModel& model) :
		CMFCPropertyPage(CModelCreditPage::IDD),
		m_model(model)
	{
		m_psp.dwFlags &= ~(PSP_HASHELP);
	}

	CModelCreditPage::~CModelCreditPage()
	{
	}

	void CModelCreditPage::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);

		DDX_Text(pDX, IDC_MODELS_CREDITTEXT, m_model.m_credit);
		DDX_Text(pDX, IDC_MODELS_COPYRIGHT, m_model.m_copyright);

	}


	BEGIN_MESSAGE_MAP(CModelCreditPage, CMFCPropertyPage)
	END_MESSAGE_MAP()

}