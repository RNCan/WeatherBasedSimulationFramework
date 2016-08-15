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

#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationUI.h"

class CExtractSSIDlg : public CDialog
{
	DECLARE_DYNCREATE(CExtractSSIDlg)

	// Construction
public:
	CExtractSSIDlg(CWnd* pParent = NULL);
	~CExtractSSIDlg();


	CString m_gridFilePath;

	BOOL m_bExtractElev;
	BOOL m_bExtractSlopeAspect;
	BOOL m_bMissingOnly;
	BOOL m_bExtractOther;
	int m_interpolationType;

protected:

	enum { IDD = IDD_SIM_LOC_EXTRACT };


	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	DECLARE_MESSAGE_MAP()

	CMFCEditBrowseCtrlEx		m_gridFilePathCtrl;

};

