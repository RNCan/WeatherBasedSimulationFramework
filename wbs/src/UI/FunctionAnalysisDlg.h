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


#include "Simulation/FunctionAnalysis.h"
#include "UI/Common/UGEditCtrl.h"
#include "UI/ModelPages.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CDimensionGridCtrl : public CUGCtrl
	{
	public:

		enum TColumn { CL, CT, CV, NB_COLS };
		static const int COL_DIM[NB_COLS];
		static const int COL_SIZE[NB_COLS];
		static int MinimumRows();

		virtual void OnSetup();
		virtual void OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed);
		void Initialize(const WBSF::StringVector& variables);


		virtual int OnCanSizeRow(long row) { UNREFERENCED_PARAMETER(row); return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual int OnMenuStart(int col, long row, int section);
		virtual void OnMenuCommand(int col, long row, int section, int item);
		virtual void OnKeyDown(UINT *vcKey, BOOL processed);

	private:

		afx_msg LRESULT OnCopy(WPARAM, LPARAM);
		DECLARE_MESSAGE_MAP()
	};



	typedef CModelOutputVariableCtrl CFunctionVariableCtrl;


	/////////////////////////////////////////////////////////////////////////////
	// CFunctionAnalysisDlg dialog

	class CFunctionAnalysisDlg : public CDialog
	{
		// Construction
	public:
		CFunctionAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor



		CExecutablePtr GetExecutable()const{ return m_analysis.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_analysis = GetAnalysis(pExecute); }
		CFunctionAnalysis & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CFunctionAnalysis&>(*pItem); }



	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_FUNCTION_ANALYSIS };

		CDimensionGridCtrl m_dimensionCtrl;
		CFunctionVariableCtrl m_functionCtrl;

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		afx_msg void OnTest();
		afx_msg void OnHelp();


		ERMsg Compile();

		DECLARE_MESSAGE_MAP()

		CFunctionAnalysis m_analysis;
		CExecutablePtr m_pParent;

	public:
		afx_msg void OnStnDblclickVariableList();
	};

}