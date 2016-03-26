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
#include "ModelBase/ModelInput.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CInputVariableGridCtrl : public CUGEditCtrl
	{
	public:

		virtual void OnSetup();
		void Initialize(const CStringArray& variables);

		//virtual int OnCanSizeRow(long row) {UNREFERENCED_PARAMETER(row);return FALSE; }
		//virtual int OnCanSizeTopHdg() {return FALSE; }


		void GetData(CModelInput& modelInput);
		WBSF::StringVector m_variableVector;


	};




	class COutputGridCtrl : public CUGCtrl
	{
	public:

		virtual void OnSetup();

		void SetData(const CFunctionDefArray& data);

		virtual int OnCanSizeRow(long row) { UNREFERENCED_PARAMETER(row); return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual int OnCanSizeSideHdg() { return FALSE; }


		ERMsg Execute(const CModelInput& modelInput);
		//virtual int OnMenuStart(int col,long row,int section);
		//virtual void OnMenuCommand(int col, long row, int section, int item);

		CFunctionDefArray m_functionVector;

	};

	/////////////////////////////////////////////////////////////////////////////
	// CFunctionAnalysisTestDlg dialog

	class CFunctionAnalysisTestDlg : public CDialog
	{
		// Construction
	public:
		CFunctionAnalysisTestDlg(CWnd* pParentWnd);   // standard constructor


		void SetFunctionArray(CFunctionDefArray& functionVector);

		//CExecutablePtr GetExecutable()const{return m_analysis.CopyObject();}
		//void SetExecutable(CExecutablePtr pExecute){m_analysis = GetAnalysis(pExecute);}
		//CFunctionAnalysis & GetAnalysis(const CExecutablePtr& pItem){ ASSERT( pItem); return dynamic_cast<CFunctionAnalysis&>( *pItem ); }




	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_FUNCTION_TEST };

		CInputVariableGridCtrl m_inputCtrl;
		COutputGridCtrl m_outputCtrl;


		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

	};

}