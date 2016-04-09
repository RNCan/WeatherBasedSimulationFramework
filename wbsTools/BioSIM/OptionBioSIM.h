// OptionAdvanced.h : header file
//

#pragma once

#include "Resource.h"
/////////////////////////////////////////////////////////////////////////////
// COptionBioSIM dialog

class COptionBioSIM : public CMFCPropertyPage
{
	//DECLARE_DYNCREATE(COptionBioSIM)

// Construction
public:
	COptionBioSIM();
	~COptionBioSIM();

// Dialog Data

	enum { IDD = IDD_BIOSIM_OPTION };
	BOOL	m_bSave;
	BOOL    m_bExportAllLines;
	//BOOL	m_bAddBioKrigingButton;
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

