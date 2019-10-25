// MiniTab2CSVDlg.h 
//

#pragma once
#include "afxwin.h"
//#include "Path.h"
//#include "UtilWin.h"

class CMTWColomn
{
public:

	CMTWColomn operator =(const CMTWColomn& col)
	{
		m_name=m_name;
		m_no=m_no;
		m_nbNoData=m_nbNoData;

		m_data.Copy(m_data);
		
	}

	CStringA m_name;
	long m_no;
	long m_nbNoData;

	CArray<float> m_data;
};

typedef CArray<CMTWColomn, CMTWColomn&>CMTWColomnArray;


// CMiniTab2CSVDlg 
class CMiniTab2CSVDlg : public CDialogEx
{

public:
	CMiniTab2CSVDlg(CWnd* pParent = NULL);	


	enum { IDD = IDD_MINITAB2CSV_DIALOG, IDH = IDR_HTML_MINITAB2CSV_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ¤ä´©

	void OnButtonCancel();


protected:
	HICON m_hIcon;

	void MTW2CSV(const CString& filePathIn, const CString& filePathOut);
	bool ReadVersionIBM(CFile& mtw, CMTWColomnArray& colArray);
	bool ReadVersionVAX(CFile& mtw, CMTWColomnArray& colArray);
	void ReadVersion80(CFile& mtw, CMTWColomnArray& colArray);

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	

public:
	afx_msg void OnBnClickedTransform();

	CString m_inputPath;
	CString m_outputPath;
	int m_nFilterIndex;


	afx_msg void OnBnClickedInputBrowse();
	afx_msg void OnBnClickedOutputBrowse();
};
