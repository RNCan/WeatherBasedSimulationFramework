// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDailyEditorOptionsDlg
#include "UI/Common/CommonCtrl.h"
#include "UI/OptionLinks.h"
#include "UI/OptionRegional.h"



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

class CDailyEditorOptionsDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CDailyEditorOptionsDlg)

    enum TPane{LAST_OPEN=-1, LINKS, TIME_FORMAT};

// Construction
public:
	CDailyEditorOptionsDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);
	virtual ~CDailyEditorOptionsDlg();

// Attributes
public:

    WBSF::COptionLinks m_links;
	WBSF::COptionRegional m_timeFormat;
	COptionCharts m_charts;
	CImageList m_images;
	//CImage	  m_image;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDailyEditorOptionsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:


	// Generated message map functions
protected:

	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
	
	void OnFinish();

private:

	HICON	m_hIcon;

    
public:
	virtual BOOL OnInitDialog();
};
