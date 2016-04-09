#pragma once


#include "Basic/NormalsDatabase.h"

class CNormalsEditorDoc : public CDocument
{
protected: // création à partir de la sérialisation uniquement

	CNormalsEditorDoc();
	DECLARE_DYNCREATE(CNormalsEditorDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, STATION_INDEX_CHANGE, LOCATION_CHANGE, OUTPUT_CHANGE, CLOSE, LANGUAGE_CHANGE,
		STATION_LIST_PROPERTIES_FILTERS_CHANGE, DATA_PROPERTIES_EDITION_MODE_CHANGE,
		PROPERTIES_TM_CHANGE, NB_EVENTS
	};


	static const char* DOCUMENT_XML;

	//bool m_bEditable;


	void SetCurStationIndex(size_t i, CView* pSender=NULL);
	size_t GetCurStationIndex()const {return m_stationIndex;}
	const WBSF::CNormalsStationPtr& GetCurStation()const{ return m_pStation; }
	void SetCurStation(WBSF::CLocation& station, CView* pSender = NULL);
	WBSF::CNormalsDatabasePtr& GetDatabase(){ return m_pDatabase; }
	bool CancelDataEdition();

	//std::set<int> GetYears()const{ return m_years; }
	//void SetYears(std::set<int> in){ if (in != m_years){ m_years = in; UpdateAllViews(NULL, STATION_LIST_PROPERTIES_YEARS_CHANGE); } }
	WBSF::CWVariables GetFilters()const{ return m_filters; }
	void SetFilters(WBSF::CWVariables in){ if (in != m_filters){ m_filters = in; UpdateAllViews(NULL, STATION_LIST_PROPERTIES_FILTERS_CHANGE); } }
	bool GetDataInEdition()const{ return m_bDataInEdition; }
	void SetDataInEdition(bool in){ if (in != m_bDataInEdition){ m_bDataInEdition = in; UpdateAllViews(NULL, DATA_PROPERTIES_EDITION_MODE_CHANGE); } }
	//size_t GetStatistic()const{ return m_statistic; }
	//void SetStatistic(size_t in){ if (in != m_statistic){ m_statistic = in; UpdateAllViews(NULL, DATA_PROPERTIES_STAT_CHANGE); } }
	//CTM GetTM()const{ return m_TM; }
	//void SetTM(CTM in){ if (in != m_TM){ m_TM = in; UpdateAllViews(NULL, PROPERTIES_TM_CHANGE); } }
	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

	//int GetChartsZoom()const{ return m_chartsZoom; }
	//void SetChartsZoom(int in){ m_chartsZoom = in; UpdateAllViews(NULL, CHARTS_PROPERTIES_ZOOM_CHANGE, NULL); }
	//CTPeriod GetPeriod()const{ return m_chartsPeriod; }
	//void SetPeriod(CTPeriod in){ if (in != m_chartsPeriod){ m_chartsPeriod = in; UpdateAllViews(NULL, DATA_PROPERTIES_PERIOD_CHANGE, NULL); } }
	//bool GetPeriodEnabled()const{ return m_bPeriodEnabled; }
	//void SetPeriodEnabled(bool in){ m_bPeriodEnabled = in; UpdateAllViews(NULL, DATA_PROPERTIES_ENABLE_PERIOD_CHANGE, NULL); }

	//CWVariables GetVariables()const{ return m_variables; }
	//void SetVariables(CWVariables in){ if (in != m_variables){ m_variables = in; UpdateAllViews(NULL, DATA_PROPERTIES_VARIABLES_CHANGE); } }

	//int GetCurrentTab()const{ return m_currentTab; }
	//void SetCurrentTab(int in){ m_currentTab = in; }


	bool IsStationModified(size_t row)const;
// Opérations
public:

// Substitutions
public:
	virtual void InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc, BOOL bMakeVisible = TRUE);
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
	virtual BOOL IsModified();
	virtual void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint=NULL);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implémentation
public:
	virtual ~CNormalsEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Fonctions générées de la table des messages
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Fonction d'assistance qui définit le contenu de recherche pour un gestionnaire de recherche
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS


	
	afx_msg void OnValidation();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	


protected:


	//properties

	WBSF::CWVariables m_filters;
	bool m_bDataInEdition;
	std::string m_outputText;

	size_t m_stationIndex;
	WBSF::CNormalsStationPtr m_pStation;
	WBSF::CNormalsDatabasePtr m_pDatabase;

	std::set<size_t> m_modifiedStation;
};
