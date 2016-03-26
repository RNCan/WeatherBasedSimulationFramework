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


#include "ModelBase/Model.h"
#include "ModelBase/ModelInput.h"
#include "FileManager/FileManager.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/OpenDirEditCtrl.h"
#include "UI/WGInputDlg.h"
#include "UI/ParametersVariationsDlg.h"
// 

namespace WBSF
{

	class CModelInputDlg;
	class CModelInputListBox : public CBioSIMListBox
	{
	public:

		CModelInputListBox();
		~CModelInputListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return GetFM().ModelInput(m_model.GetExtension()); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnSelectionChanged();
		virtual void OnSetAsDefault(int iItem);
		virtual BOOL OnBeforeCopyItem(int iItem, CString newName);


		//virtual void OnEditItem(int iItem);

		//CString m_modelID;
		CModel m_model;

		ERMsg LoadModelInput(const CString& modelInputName, CModelInput& modelInput);
		ERMsg SaveModelInput(const CString& modelInputName, bool bAskConfirm = true);

		DECLARE_MESSAGE_MAP()

	protected:

		CModelInputDlg* m_pModelInputDlg;
		CModelInput m_lastModelInput;
		int m_lastSelection;


		//ERMsg SaveWGInput(const CString& curName, CWGInput& WGInput);
	};

	class CModelInputManagerDlg : public CDialog
	{
		// Construction
	public:

		CModelInputManagerDlg(CWnd* pParent = NULL);   // standard constructor
		virtual ~CModelInputManagerDlg();


		CModel m_model;
		CString m_modelInputName;

		// Implementation
	protected:

		afx_msg void OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult);
		DECLARE_MESSAGE_MAP()

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		//	virtual void OnCancel();
		virtual BOOL DestroyWindow();

		// Dialog Data
		enum { IDD = IDD_MODELINPUT_EDITOR };

	private:

		void UpdateCtrl(void);
		COpenDirEditCtrl m_filePathCtrl;

		CModelInputListBox m_fileListCtrl;
	};


	//*******************************************************************
	class CWGInputListBox : public CBioSIMListBox
	{
	public:

		CWGInputListBox();
		~CWGInputListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return GetFM().WGInput(); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnSelectionChanged();
		virtual void OnSetAsDefault(int iItem);
		virtual BOOL OnBeforeCopyItem(int iItem, CString newName);


		//virtual void OnEditItem(int iItem);

		//CString m_modelID;
		CModel m_model;

		ERMsg LoadWGInput(const CString& TGName, CWGInput& WGInput);
		ERMsg SaveWGInput(const CString& TGName, bool bAskConfirm = true);

		DECLARE_MESSAGE_MAP()

	protected:

		CWGInputDlg* m_pWGInputDlg;
		CWGInput m_lastWGInput;
		int m_lastSelection;


		//ERMsg SaveWGInput(const CString& curName, CWGInput& WGInput);
	};

	class CWGInputManagerDlg : public CDialog
	{
		// Construction
	public:

		CWGInputManagerDlg(CWnd* pParent = NULL);   // standard constructor
		virtual ~CWGInputManagerDlg();


		CModel m_model;
		CString m_WGInputName;

		// Implementation
	protected:

		afx_msg void OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult);
		DECLARE_MESSAGE_MAP()

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		//virtual void OnCancel();
		virtual BOOL DestroyWindow();

		// Dialog Data
		enum { IDD = IDD_WG_INPUT_MANAGER };

	private:

		void UpdateCtrl(void);
		COpenDirEditCtrl m_filePathCtrl;

		CWGInputListBox m_fileListCtrl;
	};


	//*******************************************************************
	class CParametersVariationsDlg;
	class CParametersVariationsListBox : public CBioSIMListBox
	{
	public:

		CParametersVariationsListBox(const CModel& model, bool bAddDefault = true, bool bShowVariationType = true);
		~CParametersVariationsListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return GetFM().PVD(m_model.GetExtension()); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnSelectionChanged();
		virtual BOOL OnBeforeCopyItem(int iItem, CString newName);

		const CModel& m_model;


		ERMsg LoadPVDInput(const CString& name, CParametersVariationsDefinition& PVD);
		ERMsg SavePVDInput(const CString& name, bool bAskConfirm = true);

		DECLARE_MESSAGE_MAP()

	protected:

		CParametersVariationsDlg* m_pPVDDlg;
		CParametersVariationsDefinition m_lastPVD;
		int m_lastSelection;


		//ERMsg SaveWGInput(const CString& curName, CWGInput& WGInput);
	};

	class CParametersVariationsManagerDlg : public CDialog
	{
		// Construction
	public:

		CParametersVariationsManagerDlg(bool bAddDefault = true, bool bShowVariationType = true, CWnd* pParent = NULL);   // standard constructor
		virtual ~CParametersVariationsManagerDlg();


		CModel m_model;
		CString m_parametersVariationsName;

		// Implementation
	protected:

		afx_msg void OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult);
		DECLARE_MESSAGE_MAP()

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		//virtual void OnCancel();
		virtual BOOL DestroyWindow();

		// Dialog Data
		enum { IDD = IDD_PARAMETERS_VARIATIONS_MANAGER };

	private:

		void UpdateCtrl(void);

		COpenDirEditCtrl m_filePathCtrl;
		CParametersVariationsListBox m_fileListCtrl;

	};

}