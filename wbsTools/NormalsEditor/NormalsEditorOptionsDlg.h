// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNormalsEditorOptionsDlg
#include "UI/Common/CommonCtrl.h"
#include "UI/OptionLinks.h"
#include "UI/OptionRegional.h"
#include "WeatherBasedSimulationUI.h"

class COptionCharts : public CMFCPropertyPage
{

	// Construction
public:
	COptionCharts();
	~COptionCharts();

	bool OnFinish();
	// Dialog Data
	enum { IDD = IDD_CMN_OPTION_CHARTS };

	CCFLEdit m_maxDataCtrl;
	CCFLEdit m_nbPixelsPerXCtrl;

	// Overrides
	// ClassWizard generate virtual function overrides
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	

	// Implementation
protected:
	// Generated message map functions
	
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()

	bool OnBrowse(CEdit& editBox, CString& fileName);

private:

	bool m_bInit;

public:
	virtual void OnOK();
};

class CNormalsEditorOptionsDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CNormalsEditorOptionsDlg)

    enum TPane{LAST_OPEN=-1, LINKS, TIME_FORMAT};

// Construction
public:
	CNormalsEditorOptionsDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);
	virtual ~CNormalsEditorOptionsDlg();

	virtual BOOL OnInitDialog();


    WBSF::COptionLinks m_links;
	WBSF::COptionRegional m_regional;
	COptionCharts m_charts;
	CImageList m_images;



	// Generated message map functions
protected:

	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
	
	void OnFinish();


	HICON	m_hIcon;

    


};

