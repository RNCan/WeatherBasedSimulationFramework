#pragma once

//#undef __AFXOLE_H__
//#include "UGExcel.h"
#include "UGCtrl.h"
#include "DailyEditorDoc.h"


typedef CUGCtrl CResultCtrlBase;

class ByPosition
{
public:

	ByPosition(int pos)
	{
		m_pos = pos;
	}
	
//	bool operator()(CStudiesDefinitions::const_reference in)const{ return in.second.m_position == m_pos; }

protected:
	
	int m_pos;
};

class CCtrlInfo
{
public:

	CCtrlInfo()
	{
		m_bNeedInit = true;
	}

	void clear()
	{
		m_fieldsPositions.clear();
		m_observations.clear();
		m_TRefPos.clear();
		m_observationsFactors.clear();
	}

	//CTRef GetTRef(long row, const CStudyData& data);
	//void UpdateObservationsDates(int col, long row, const CStudyData& data);
	//void InitObservationsDates(const CStudyData& data);
	//double GetObservationFactor(long row, const CStudyData& data);

	//std::map<int, std::string> m_positions;
	bool m_bNeedInit;
	std::string m_studyName;
	std::map<int, std::string> m_fieldsPositions;
	std::set<CTRef> m_observations;
	std::vector<size_t> m_TRefPos;
	std::vector<double> m_observationsFactors;


};

typedef std::vector<CCtrlInfo> CCtrlInfoVector;

class CResultCtrl: public CResultCtrlBase
{
public:

	//enum TDimension { LOCATION, PARAMETER, REPLICATION, TIME, NB_DIMENSION };
	enum TLocation { R_DD, R_DEG, R_MIN, R_SEC };
	enum TParamete{ VARIABLE_PARAMETERS, ALL_PARAMETERS };
	enum TTime{ R_YEAR, R_MONTH, R_DAY, R_JDAY};
	//enum TFormat{ };

	CResultCtrl();
	~CResultCtrl();


	//void SetProject(CWeatherDatabasePtr result);


	virtual void OnSetup();
	virtual void OnSheetSetup(int sheetNumber);
	virtual void OnTabSelected(int ID);
	virtual int OnMenuStart(int col, long row, int section);
	virtual void OnMenuCommand(int col, long row, int section, int item);
	virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
	virtual void OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed);
	virtual void OnCharDown(UINT *vcKey, BOOL processed);
	virtual void OnKeyUp(UINT *vcKey, BOOL processed);
	virtual int OnEditStart(int col, long row, CWnd **edit);
	virtual void OnAdjustComponentSizes(RECT *grid, RECT *topHdg, RECT *sideHdg, RECT *cnrBtn, RECT *vScroll, RECT *hScroll, RECT *tabs);
	
	//virtual void OnLClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed);
	
	using CResultCtrlBase::Paste;
	int Paste();
	void SelectRows();
	bool IsRowsSelected(const CString& str);
	//void CopyRows();
	int PasteRows(CString str);

	virtual int OnCanSizeRow(long ) { return FALSE; }
	virtual int OnCanSizeTopHdg() { return FALSE; }

	virtual void OnGetCell(int col, long row, CUGCell *cell);
	virtual void OnSetCell(int col, long row, CUGCell *cell);
	virtual void OnColSized(int col, int *width);
	virtual int OnHint(int col,long row,int section,CString *string);
	virtual int OnVScrollHint(long row,CString *string);
	virtual COLORREF OnGetDefBackColor(int section);

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	std::string GetActiveTabName();
	void InitSheet(const CCtrlInfo& info);
	void DeleteSelection();

protected:

	
	DECLARE_MESSAGE_MAP()
	
	CCtrlInfoVector m_info;

	CFont m_font;
	CPen m_cellBorderPen;
	//CExcelTopHdg	m_excelTopHdg;//a faire mémoire...
	//CExcelSideHdg m_excelSideHdg;


	BOOL PreTranslateMessage(MSG* pMsg);
};
