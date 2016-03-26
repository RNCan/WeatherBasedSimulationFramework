// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWeatherUpdaterOptionsDlg
#include "UI/Common/CommonCtrl.h"
#include "UI/OptionLinks.h"
#include "UI/OptionRegional.h"
#include "WeatherBasedSimulationUI.h"
//
//class COptionCharts : public CMFCPropertyPage
//{
//
//	// Construction
//public:
//	COptionCharts();
//	~COptionCharts();
//
//	bool OnFinish();
//	// Dialog Data
//	enum { IDD = IDD_CMN_OPTION_CHARTS };
//
//	CCFLEdit m_maxDataCtrl;
//	CCFLEdit m_nbPixelsPerXCtrl;
//
//	// Overrides
//	// ClassWizard generate virtual function overrides
//	
//protected:
//	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
//	
//
//	// Implementation
//protected:
//	// Generated message map functions
//	
//	virtual BOOL OnInitDialog();
//	
//	DECLARE_MESSAGE_MAP()
//
//	bool OnBrowse(CEdit& editBox, CString& fileName);
//
//private:
//
//	bool m_bInit;
//
//public:
//	virtual void OnOK();
//};

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

