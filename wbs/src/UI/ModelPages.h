//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once


#include <deque>
#include "Basic/WeatherDefine.h"
#include "ModelBase/Model.h"
#include "UI/Common/PropertiesListBox.h"
#include "UI/WVariablesEdit.h"
#include "UI/ModelFormDlg.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	class CModelInputDlg;

	//*************************************************************************************
	// CModelGeneralPage 

	class CModelBrowseEdit : public CMFCEditBrowseCtrl
	{
	public:

		void Init(const CString& path)
		{
			m_path = path;
			EnableBrowseButton();
		}

	protected:

		virtual void OnBrowse()
		{

			CFileDialog openDialog(true, _T(""), _T(""), OFN_NOCHANGEDIR, _T("DLL (*.dll)|*.dll|Executable (*.exe)|*.exe||"), this);
			openDialog.m_ofn.lpstrInitialDir = m_path;

			if (openDialog.DoModal() == IDOK)
			{
				SetWindowText(openDialog.GetFileName());
			}
		}

		CString m_path;
	};


	class CModelGeneralPage : public CMFCPropertyPage
	{

	public:

		CModelGeneralPage(CModel& model);
		~CModelGeneralPage();



	protected:

		enum { IDD = IDD_MODELS_GENERAL };

		CModelBrowseEdit	m_DLLNameCtrl;
		CEdit				m_DLLVersionCtrl;

		CModel& m_model;

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		void UpdateDLLVersion();


		afx_msg void OnDLLNameChange();
		DECLARE_MESSAGE_MAP()
	};


	//****************************************************************************************
	// CModelWGInputPage
	class CModelWGInputPage : public CMFCPropertyPage
	{
	public:
		CModelWGInputPage(CModel& model);
		~CModelWGInputPage();

		enum { IDD = IDD_MODELS_WG_INPUT };

	protected:

		CWVariablesEdit m_variablesCtrl;
		CModel& m_model;

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()

		afx_msg void OnTransferFileVersionChange();
	};


	//****************************************************************************************
	//CModelSSIPage
	class CModelSSIPage : public CMFCPropertyPage
	{
	public:

		CModelSSIPage(CModel& model);
		~CModelSSIPage();

		enum { IDD = IDD_MODELS_SSI };

	protected:

		CVSListBox m_SSICtrl;

		CModel& m_model;

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()

		//afx_msg void OnTransferFileVersionChange();
	};


	//*************************************************************************************
	// CModelInputPage 




	class CRectProp : public CCFLPropertyGridProperty
	{
	public:
		CRectProp(const CString& strGroupName, DWORD_PTR dwData = 0, BOOL bIsValueList = FALSE);

		virtual BOOL OnUpdateValue();
		virtual void SetValue(const COleVariant& varValue);
		virtual const COleVariant& GetValue() const;
	};

	class CBoundedNumberSubProp : public CMFCPropertyGridProperty
	{
	public:
		CBoundedNumberSubProp(const CString& strName, const COleVariant& varValue, /*int nMinValue, int nMaxValue, */LPCTSTR lpszDescr = NULL);

		virtual BOOL OnUpdateValue();
	};




	typedef std::unique_ptr<CModelInputParameterDef>CModelInputParameterDefPtr;
	typedef std::deque<CModelInputParameterDefPtr>CModelInputParameterDefPtrDeque;

	class CModelInputParameterCtrl : public CPropertiesListBox
	{
	public:

		CModelInputParameterCtrl();

		void HideMember(int memberID);
		void ShowForm(bool bShow);

		void GetData(CModelInputParameterDefVector& data);
		void SetData(const CModelInputParameterDefVector& data);

		void SetFormRect(const CRect& formRect)
		{
			ASSERT(m_form.GetSafeHwnd());
			m_form.SetWindowPos(NULL, 0, 0, formRect.Width(), formRect.Height(), SWP_NOMOVE | SWP_NOZORDER);
			//m_formWidth=formRect.Width(); m_formHeight=formRect.Height();
		}
		CRect GetFormRect()const
		{
			CRect rect;
			m_form.GetWindowRect(rect);
			rect -= rect.TopLeft();
			return rect;
		}

		void OnItemActivation(const CString& name);
		void UpdateProperties(const CString& name);
		void UpdateForm();
		void TestForm();
	protected:

		virtual CWnd* OnCreateProperties();
		virtual BOOL OnBeforeRemoveItem(int iItem);
		virtual void OnAfterAddItem(int iItem);
		virtual void OnAfterRenameItem(int iItem);
		virtual void OnGetDataFromProperties(int iItem);
		virtual void OnSetDataToProperties(int iItem);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual void OnSelectionChanged();

		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()




		int FindName(const CString& name);


		CModelInputParameterDefPtrDeque m_data;
		CModelFormDialog m_form;

		short m_lastType;

		CModelInputDlg* m_pModelInput;
	};


	class CModelInputPage : public CMFCPropertyPage
	{
	public:

		CModelInputPage(CModel& model);
		~CModelInputPage();

		enum { IDD = IDD_MODELS_INPUT };

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnSetActive();
		virtual BOOL OnKillActive();


		// Implementation
	protected:
		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg void OnTest();

		CModelInputParameterCtrl m_inputParamCtrl;
		CModel& m_model;



	};


	//*************************************************************************************
	// CModelOutputPage 
	class CModelOutputVariableCtrl : public CPropertiesListBox
	{
	public:

		CModelOutputVariableCtrl();

		void HideMember(int memberID);
		void GetData(CModelOutputVariableDefVector& data);
		void SetData(const CModelOutputVariableDefVector& data);
		void Add(const CModelOutputVariableDef& data);

	protected:

		virtual CWnd* OnCreateProperties();
		virtual BOOL OnBeforeRemoveItem(int iItem);
		virtual void OnAfterAddItem(int iItem);
		virtual void OnAfterRenameItem(int iItem);
		virtual void OnGetDataFromProperties(int iItem);
		virtual void OnSetDataToProperties(int iItem);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;

		CModelOutputVariableDefVector m_data;

		DECLARE_MESSAGE_MAP()

	};



	class CModelOutputPage : public CMFCPropertyPage
	{

	public:
		CModelOutputPage(CModel& model);
		~CModelOutputPage();

	protected:

		enum { IDD = IDD_MODELS_OUTPUT };


		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()


		//CEdit	  m_missingValueCtrl;
		CComboBox m_outputTypeCtrl;
		CComboBox m_outputModeCtrl;
		CModelOutputVariableCtrl m_outputVarCtrl;

		CModel& m_model;
	};


	//*************************************************************************************
	// CModelCreditPage 

	class CModel;
	class CModelCreditPage : public CMFCPropertyPage
	{
	public:
		CModelCreditPage(CModel& model);
		~CModelCreditPage();

	protected:

		enum { IDD = IDD_MODELS_CREDIT };
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


		CModel& m_model;

		//	afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()

	};

}