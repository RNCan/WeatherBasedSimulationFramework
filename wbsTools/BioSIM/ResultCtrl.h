#pragma once

#include "UltimateGrid/UGExcel.h"
#include "Simulation\Result.h"


class CResultCtrl : public CUGExcel
{
public:

	enum TLocation { R_DD, R_DEG, R_MIN, R_SEC };
	enum TParamete{ VARIABLE_PARAMETERS, ALL_PARAMETERS };
	enum TTime{ R_YEAR, R_MONTH, R_DAY, R_JDAY};

	CResultCtrl();
	~CResultCtrl();

	bool m_bIsExecute;

	virtual void OnSetup();
	virtual void OnSheetSetup(int ID);
	
	void Update();
	void SetData(WBSF::CResultPtr result);
	void SetStatType(size_t type){ m_statType = type; Invalidate(); UpdateWindow(); }
	int OnMenuStart(int col,long row,int section);
	//void OnMenuCommand(int col, long row, int section, int item);

	virtual int OnCanSizeTopHdg(){ return FALSE; }
	virtual int OnCanSizeSideHdg(){ return TRUE; }
	virtual int OnCanSizeRow(long row){ return FALSE; }
	virtual int OnCanSizeCol(int col){ return TRUE;}
	virtual void OnColSized(int col, int *width);
	virtual int  OnSideHdgSized(int *width);
	
	virtual int OnHint(int col,long row,int section,CString *string);
	virtual int OnVScrollHint(long row,CString *string);
	virtual void OnGetCell(int col,long row, CUGCell *cell);


	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* /*pScrollBar*/);

protected:


	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateToolBar(CCmdUI *pCmdUI);
	afx_msg void OnEditCopy();


	size_t m_statType;
	WBSF::StringVector m_dataTitles;
	WBSF::CResultPtr m_pResult;
	CFont m_font;
	CPen m_cellBorderPen;


	//last
	__time64_t m_lastRunTime;

};
