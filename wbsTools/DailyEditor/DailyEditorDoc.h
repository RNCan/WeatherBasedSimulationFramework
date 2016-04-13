#pragma once


#include "Basic/DailyDatabase.h"

class CDailyEditorDoc : public CDocument
{

	DECLARE_DYNCREATE(CDailyEditorDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, STATION_INDEX_CHANGE, LOCATION_CHANGE, OUTPUT_CHANGE, CLOSE, LANGUAGE_CHANGE,
		STATION_LIST_PROPERTIES_YEARS_CHANGE, STATION_LIST_PROPERTIES_FILTERS_CHANGE,
		DATA_PROPERTIES_EDITION_MODE_CHANGE, DATA_PROPERTIES_STAT_CHANGE, 
		CHARTS_PROPERTIES_ZOOM_CHANGE, DATA_PROPERTIES_ENABLE_PERIOD_CHANGE, DATA_PROPERTIES_PERIOD_CHANGE,
		DATA_PROPERTIES_VARIABLES_CHANGE,
		PROPERTIES_TM_CHANGE, NB_EVENTS
	};


	static const char* DOCUMENT_XML;

	CDailyEditorDoc();
	virtual ~CDailyEditorDoc();

	void SetCurStationIndex(size_t i, CView* pSender=NULL);
	size_t GetCurStationIndex()const {return m_stationIndex;}
	const WBSF::CWeatherStationPtr& GetCurStation()const{ return m_pStation; }
	void SetCurStation(WBSF::CLocation& station, CView* pSender = NULL);
	WBSF::CWeatherDatabasePtr& GetDatabase(){ return m_pDatabase; }
	bool CancelDataEdition();

	std::set<int> GetYears()const{ return m_years; }
	void SetYears(std::set<int> in){ if (in != m_years){ m_years = in; UpdateAllViews(NULL, STATION_LIST_PROPERTIES_YEARS_CHANGE); } }
	WBSF::CWVariables GetFilters()const{ return m_filters; }
	void SetFilters(WBSF::CWVariables in){ if (in != m_filters){ m_filters = in; UpdateAllViews(NULL, STATION_LIST_PROPERTIES_FILTERS_CHANGE); } }
	bool GetDataInEdition()const{ return m_bDataInEdition; }
	void SetDataInEdition(bool in){ if (in != m_bDataInEdition){ m_bDataInEdition = in; UpdateAllViews(NULL, DATA_PROPERTIES_EDITION_MODE_CHANGE); } }
	size_t GetStatistic()const{ return m_statistic; }
	void SetStatistic(size_t in){ if (in != m_statistic){ m_statistic = in; UpdateAllViews(NULL, DATA_PROPERTIES_STAT_CHANGE); } }
	WBSF::CTM GetTM()const{ return m_TM; }
	void SetTM(WBSF::CTM in){ if (in != m_TM){ m_TM = in; UpdateAllViews(NULL, PROPERTIES_TM_CHANGE); } }
	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

	int GetChartsZoom()const{ return m_chartsZoom; }
	void SetChartsZoom(int in){ m_chartsZoom = in; UpdateAllViews(NULL, CHARTS_PROPERTIES_ZOOM_CHANGE, NULL); }
	WBSF::CTPeriod GetPeriod()const{ return m_period; }
	void SetPeriod(WBSF::CTPeriod in){ if (in != m_period){ m_period = in; UpdateAllViews(NULL, DATA_PROPERTIES_PERIOD_CHANGE, NULL); } }
	bool GetPeriodEnabled()const{ return m_bPeriodEnabled; }
	void SetPeriodEnabled(bool in){ m_bPeriodEnabled = in; UpdateAllViews(NULL, DATA_PROPERTIES_ENABLE_PERIOD_CHANGE, NULL); }

	WBSF::CWVariables GetVariables()const{ return m_variables; }
	void SetVariables(WBSF::CWVariables in){ if (in != m_variables){ m_variables = in; UpdateAllViews(NULL, DATA_PROPERTIES_VARIABLES_CHANGE); } }

	int GetCurrentTab()const{ return m_currentTab; }
	void SetCurrentTab(int in){ m_currentTab = in; }


	bool IsStationModified(size_t row)const;

	bool IsExecute()const{ return m_bExecute; }
	void OnInitialUpdate();
	
	
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
	virtual BOOL IsModified();
	virtual void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint=NULL);



	

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnValidation();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);



	//properties
	std::set<int> m_years;
	WBSF::CWVariables m_filters;
	bool m_bDataInEdition;
	size_t m_statistic;
	WBSF::CTM m_TM;
	std::string m_outputText;
	WBSF::CTPeriod m_period;
	bool m_bPeriodEnabled;
	int m_chartsZoom;
	int m_currentTab;
	WBSF::CWVariables m_variables;
	bool m_bExecute;

	size_t m_stationIndex;
	WBSF::CWeatherStationPtr m_pStation;
	WBSF::CWeatherDatabasePtr m_pDatabase;
	

	std::set<size_t> m_modifiedStation;

	static UINT OpenDatabase(void* pParam);


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif



#ifdef SHARED_HANDLERS
	// Fonction d'assistance qui d�finit le contenu de recherche pour un gestionnaire de recherche
	void SetSearchContent(const CString& value);
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);

#endif // SHARED_HANDLERS



};
