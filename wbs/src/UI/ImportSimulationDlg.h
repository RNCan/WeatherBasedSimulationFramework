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

#include "Simulation/ImportSimulation.h"
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

		void SetImportHeader(const CString& header);
		void GetData(CColumnLinkVector& data);
		void SetData(const CColumnLinkVector& data);

		virtual int OnCellTypeNotify(long ID, int col, long row, long msg, long param);
		static short GetAutoSelect(CString header, short& extra);
		void OnDimensionChange(int row, int dimensionRef);

		virtual int OnCanSizeRow(long row) { UNREFERENCED_PARAMETER(row); return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }

	private:

		void GetImportFileFromInterface();
		void SetImportFileToInterface();

		CString GetDimensionRefText(int dimensionRef)const;
		CString GetDimensionFieldText(int dimensionRef, int dimensionField)const;
		int GetDimensionRef(CString str)const;
		int GetDimensionField(int dimensionRef, CString str)const;


		CStringArrayEx DIMENSION_LABLE;
		CStringArrayEx LOC_LABLE;
		CStringArrayEx TIME_LABLE;
		CStringArrayEx VARIABLES_LABLE;

		//	virtual int OnMenuStart(int col,long row,int section);
		//	virtual void OnMenuCommand(int col, long row, int section, int item);
	};

	// CImportSimulationDlg dialog

	class CImportSimulationDlg : public CDialog
	{
		DECLARE_DYNAMIC(CImportSimulationDlg)

	public:

		friend CImportSimulation;

		CImportSimulationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor
		virtual ~CImportSimulationDlg();
		virtual BOOL OnInitDialog();

		virtual void SetExecutable(CExecutablePtr pExecutable){ m_importSimulation = GetImportSimulation(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return m_importSimulation.CopyObject(); }

		// Dialog Data
		enum { IDD = IDD_SIM_IMPORT_SIMULATION };

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();

		DECLARE_MESSAGE_MAP()

		CImportVariablesCtrl m_columnLink;
		CCFLComboBox m_fileNameCtrl;
		COpenDirEditCtrl	m_defaultDirCtrl;
		CEdit m_internalNameCtrl;

		void FillFileName();



		void GetImportFileFromInterface();
		void SetImportFileToInterface();

		CImportSimulation m_importSimulation;
		CImportSimulation& GetImportSimulation(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CImportSimulation&>(*pItem); }

		afx_msg void OnFileNameChange();
	};

}