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

#include "AnalysisDlg.h"
#include "FileManager/FileManager.h"

#include "WeatherBasedSimulationString.h"

using namespace UtilWin;


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{
	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisDlg

	IMPLEMENT_DYNAMIC(CAnalysisDlg, CMFCPropertySheet)

	CAnalysisDlg::CAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CMFCPropertySheet(IDS_SIM_ANALYSIS_EDITOR, pParentWnd)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
		m_psh.dwFlags &= ~(PSH_HASHELP);
		m_generalPage.m_psp.dwFlags &= ~(PSP_HASHELP);
		m_wherePage.m_psp.dwFlags &= ~(PSP_HASHELP);
		m_whenPage.m_psp.dwFlags &= ~(PSP_HASHELP);
		m_whatPage.m_psp.dwFlags &= ~(PSP_HASHELP);
		m_whichPage.m_psp.dwFlags &= ~(PSP_HASHELP);
		m_howPage.m_psp.dwFlags &= ~(PSP_HASHELP);


		m_generalPage.Initialise(pParent, m_analysis);
		m_wherePage.Initialise(pParent, m_analysis);
		m_whenPage.Initialise(pParent, m_analysis);
		m_whatPage.Initialise(pParent, m_analysis);
		m_whichPage.Initialise(pParent, m_analysis);
		m_howPage.Initialise(pParent, m_analysis);

		SetLook(PropSheetLook_Tabs);

		SetIconsList(IDB_SIM_ANALYSIS_PAGES, 16, RGB(152, 152, 152));


		AddPage(&m_generalPage);
		AddPage(&m_wherePage);
		AddPage(&m_whenPage);
		AddPage(&m_whatPage);
		AddPage(&m_whichPage);
		AddPage(&m_howPage);
	}

	CAnalysisDlg::~CAnalysisDlg()
	{
	}


	BEGIN_MESSAGE_MAP(CAnalysisDlg, CMFCPropertySheet)

	END_MESSAGE_MAP()


	BOOL CAnalysisDlg::OnInitDialog()
	{
		BOOL bResult = CMFCPropertySheet::OnInitDialog();

		ModifyStyleEx(0, WS_EX_CONTROLPARENT);

		return bResult;
	}


	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisDlg message handlers 


	void CAnalysisDlg::OnDrawPageHeader(CDC* pDC, int nPage, CRect rectHeader)
	{
		rectHeader.top += 2;
		rectHeader.right -= 2;
		rectHeader.bottom -= 2;

		pDC->FillRect(rectHeader, &afxGlobalData.brBtnFace);
		pDC->Draw3dRect(rectHeader, afxGlobalData.clrBtnShadow, afxGlobalData.clrBtnShadow);

		CDrawingManager dm(*pDC);
		dm.DrawShadow(rectHeader, 2);

		CString strText;
		strText.Format(_T("Page %d description..."), nPage + 1);

		CRect rectText = rectHeader;
		rectText.DeflateRect(10, 0);

		CFont* pOldFont = pDC->SelectObject(&afxGlobalData.fontBold);
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(afxGlobalData.clrBtnText);

		pDC->DrawText(strText, rectText, DT_SINGLELINE | DT_VCENTER);

		pDC->SelectObject(pOldFont);
	}


}