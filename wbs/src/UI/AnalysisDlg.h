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

#include "AnalysisPages.h"
#include "Simulation/Analysis.h"


namespace WBSF
{
	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisDlg

	class CAnalysisDlg : public CMFCPropertySheet
	{
		DECLARE_DYNAMIC(CAnalysisDlg)

		// Construction
	public:
		CAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);
		virtual ~CAnalysisDlg();

		BOOL OnInitDialog();
		//BOOL OnCommand(WPARAM wParam, LPARAM lParam);

		CExecutablePtr GetExecutable()const{ return m_analysis.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_analysis = GetAnalysis(pExecute); }
		CAnalysis& GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CAnalysis&>(*pItem); }

		// Attributes
	protected:
		CAnalysisGeneralPage m_generalPage;
		CAnalysisWherePage m_wherePage;
		CAnalysisWhenPage m_whenPage;
		CAnalysisWhatPage m_whatPage;
		CAnalysisWhichPage m_whichPage;
		CAnalysisHowPage m_howPage;

		// Operations
	public:

		// Overrides
		virtual void OnDrawPageHeader(CDC* pDC, int nPage, CRect rectHeader);

		//void OnOK( );
		// Implementation
	public:


	protected:
		DECLARE_MESSAGE_MAP()

		CImageList m_imageList;
		HICON m_hIcon;

		CAnalysis m_analysis;

	};

}