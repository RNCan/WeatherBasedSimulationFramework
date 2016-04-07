// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWeatherUpdaterOptionsDlg
#include "UI/Common/CommonCtrl.h"
#include "UI/OptionLinks.h"
#include "UI/OptionRegional.h"
#include "WeatherBasedSimulationUI.h"


class CWeatherUpdaterOptionsDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CWeatherUpdaterOptionsDlg)

    enum TPane{LAST_OPEN=-1, LINKS, TIME_FORMAT};

// Construction
public:

	CWeatherUpdaterOptionsDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);
	virtual ~CWeatherUpdaterOptionsDlg();

	

// Attributes
public:

    WBSF::COptionLinks m_links;
	WBSF::COptionRegional m_regional;
	
	CImageList m_images;
	

	// Generated message map functions
protected:

	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	
	virtual BOOL OnInitDialog();
	void OnFinish();

	HICON	m_hIcon;

    


};

