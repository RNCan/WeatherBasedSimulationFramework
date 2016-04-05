// OptionAdvanced.h : header file
//

#pragma once

#include "Resource.h"
/////////////////////////////////////////////////////////////////////////////
// COptionsDPT dialog

class COptionsDPT : public CMFCPropertyPage
{
	//DECLARE_DYNCREATE(COptionsDPT)

// Construction
public:
	COptionsDPT();
	~COptionsDPT();

// Dialog Data

	enum { IDD = IDD_DPT_OPTION };
	BOOL	m_bSave;
	BOOL    m_bExportAllLines;
	BOOL	m_bAddClimZoneButton;

// Overrides
	// ClassWizard generate virtual function overrides
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Generated message map functions
	DECLARE_MESSAGE_MAP()
 
};

