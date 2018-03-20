#pragma once


#include "Basic/NormalsDatabase.h"

class CNormalsEditorDoc : public CDocument
{
protected: // création à partir de la sérialisation uniquement


	DECLARE_DYNCREATE(CNormalsEditorDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, STATION_INDEX_CHANGE, LOCATION_CHANGE, OUTPUT_CHANGE, LANGUAGE_CHANGE,
		STATION_LIST_PROPERTIES_FILTERS_CHANGE, DATA_PROPERTIES_EDITION_MODE_CHANGE,
		PROPERTIES_TM_CHANGE, NB_EVENTS
	};


	static const char* DOCUMENT_XML;
	CNormalsEditorDoc();
	virtual ~CNormalsEditorDoc();



	void SetCurStationIndex(size_t i, CView* pSender = NULL, bool bSendUpdate=true);
	size_t GetCurStationIndex()const {return m_stationIndex;}
	const WBSF::CNormalsStationPtr& GetCurStation()const{ return m_pStation; }
	void SetCurStation(WBSF::CLocation& station, CView* pSender = NULL);
	WBSF::CNormalsDatabasePtr& GetDatabase(){ return m_pDatabase; }
	bool CancelDataEdition();

	WBSF::CWVariables GetFilters()const{ return m_filters; }
	void SetFilters(WBSF::CWVariables in){ if (in != m_filters){ m_filters = in; UpdateAllViews(NULL, STATION_LIST_PROPERTIES_FILTERS_CHANGE); } }
	bool GetDataInEdition()const{ return m_bDataInEdition; }
	void SetDataInEdition(bool in){ if (in != m_bDataInEdition){ m_bDataInEdition = in; UpdateAllViews(NULL, DATA_PROPERTIES_EDITION_MODE_CHANGE); } }
	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }


	bool IsStationModified(size_t row)const;
	bool IsExecute()const{ return m_bExecute; }
	//void OnInitialUpdate();

	
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
	virtual BOOL IsModified();
	
	//UpdateAllViews is not virtual
	void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint=NULL);

	
protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnValidation();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	

	//properties
	WBSF::CWVariables m_filters;
	bool m_bDataInEdition;
	std::string m_outputText;

	size_t m_stationIndex;
	WBSF::CNormalsStationPtr m_pStation;
	WBSF::CNormalsDatabasePtr m_pDatabase;

	std::set<size_t> m_modifiedStation;
	bool m_bExecute;


	static UINT OpenDatabase(void* pParam);

#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
	void SetSearchContent(const CString& value);

#endif // SHARED_HANDLERS

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};
