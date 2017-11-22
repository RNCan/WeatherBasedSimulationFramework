// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CBioSIMOptionDlg
#include "OptionBioSIM.h"
#include "UI/OptionDir.h"
#include "UI/OptionLinks.h"
#include "UI/OptionRegional.h"
#include "UI/OptionAdvanced.h"
//#include "OptionErrorLevel.h"



class CBioSIMOptionDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CBioSIMOptionDlg)

// Construction
public:

	enum TPane{ LAST_OPEN = -1, BIOSIM, DIR, LINKS, REGIONAL, ADVANCED };


	CBioSIMOptionDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);

	virtual ~CBioSIMOptionDlg();
	virtual BOOL OnInitDialog();
// Attributes
public:

	
	COptionBioSIM m_BioSIM;
	WBSF::COptionDir m_directory;
    WBSF::COptionLinks m_links;
	WBSF::COptionRegional m_regional;
	WBSF::COptionAdvanced m_advanced;

	// Generated message map functions
protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();

	HICON	m_hIcon;


};

