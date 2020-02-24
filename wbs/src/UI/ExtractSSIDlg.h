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

class CExtractSSIDlg : public CDialogEx
{
	DECLARE_DYNCREATE(CExtractSSIDlg)

	// Construction
public:


	enum TExtractFrom { FROM_DEM, FROM_WEB };
	CExtractSSIDlg(CWnd* pParent = NULL);
	~CExtractSSIDlg();


	CString m_gridFilePath;

	TExtractFrom m_extractFrom;
	BOOL m_bExtractElev;
	BOOL m_bExtractSlopeAspect;
	BOOL m_bMissingOnly;
	BOOL m_bExtractShoreDistance;
	BOOL m_bExtractWebElevation;
	int m_webElevProduct;
	int m_interpolationType;
	BOOL m_bExtractWebName;
	BOOL m_bExtractWebState;
	BOOL m_bExtractWebCountry;

protected:

	enum { IDD = IDD_SIM_LOC_EXTRACT };


	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	TExtractFrom GetExtractFrom()const;
	void SetExtractFrom(TExtractFrom no);
	void UpdateCtrl();


	// Generated message map functions
	DECLARE_MESSAGE_MAP()

	CMFCEditBrowseCtrlEx		m_gridFilePathCtrl;

};

