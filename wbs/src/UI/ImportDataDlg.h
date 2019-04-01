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

#include "Simulation/ImportData.h"
#include "UI/Common/UGEditCtrl.h"
#include "UI/Common/OpenDirEditCtrl.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{


	class CImportVariablesCtrl : public CUGCtrl
	{
	public:

		virtual void OnSetup();

		void SetImportHeader(const std::string& header);
		void GetData(CColumnLinkVector& data);
		void SetData(const CColumnLinkVector& data);

		virtual int OnCellTypeNotify(long ID, int col, long row, long msg, LONG_PTR param);
		static void GetAutoSelect(const std::string& header, size_t& dimensionRef, size_t& dimensionField);


		void OnDimensionChange(int row, size_t dimensionRef);

		virtual int OnCanSizeRow(long row) { UNREFERENCED_PARAMETER(row); return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual void OnColSized(int col, int *width);
		virtual int  OnSideHdgSized(int *width);


	private:

		CString GetDimensionText(size_t dimension)const;
		CString GetFieldText(size_t dimension, size_t field)const;
		size_t GetDimension(CString str)const;
		size_t GetField(size_t dimension, CString str)const;


		CStringArrayEx DIMENSION_LABLE;
		CStringArrayEx LOC_LABLE;
		CStringArrayEx TIME_LABLE;
		CStringArrayEx VARIABLES_LABLE;
	};

	// CImportDataDlg dialog

	class CImportDataDlg : public CDialog
	{
		DECLARE_DYNAMIC(CImportDataDlg)

	public:

		friend CImportData;

		CImportDataDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor
		virtual ~CImportDataDlg();
		virtual BOOL OnInitDialog();


		virtual void SetExecutable(CExecutablePtr pExecutable){ m_importData = GetImportData(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return m_importData.CopyObject(); }

		// Dialog Data
		enum { IDD = IDD_SIM_IMPORT_SIMULATION };

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		

		DECLARE_MESSAGE_MAP()

		void OnSize(UINT nType, int cx, int cy);
		void AdjustLayout();


		
		CCFLComboBox m_nameCtrl;
		CCFLComboBox m_descriptionCtrl;
		CCFLComboBox m_fileNameCtrl;
		CImportVariablesCtrl m_columnLink;

		COpenDirEditCtrl	m_defaultDirCtrl;
		CCFLEdit m_internalNameCtrl;

		void FillFileName();


		CImportData m_importData;
		CImportData& GetImportData(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CImportData&>(*pItem); }

		afx_msg void OnFileNameChange();
		afx_msg void OnDestroy();
	};

}