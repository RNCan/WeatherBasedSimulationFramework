// TDateDlg.h : header file
//
#pragma once


#include <afxdtctl.h>
#include "TaskBarDialog.h"
#include "Basic/ERMsg.h"
/////////////////////////////////////////////////////////////////////////////
// CTDateDlg dialog
class CDailyFileFormat;


class CTDateDlg : public CTaskBarDialog
{
// Construction
public:
	CTDateDlg(void);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TDATE_DIALOG };
	CDateTimeCtrl	m_standard;
	CEdit	m_julian;

	public:
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL );
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnChangeDateStandard(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeDatejulien();
	afx_msg void OnClose();
	afx_msg void OnShow();
	afx_msg void OnDestroy();
	afx_msg void OnAboutbox();
	afx_msg void OnConvertFile();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	

	DECLARE_MESSAGE_MAP()


    void SetCurrentDate(void);
	ERMsg ConvertFile(const CString& filePath);
	ERMsg AutoDetectElement(const CStdioFile& file, CDailyFileFormat& format);

    bool m_bChangeStadard;
    bool m_bChangeJulian;
    int m_nDate;
};

