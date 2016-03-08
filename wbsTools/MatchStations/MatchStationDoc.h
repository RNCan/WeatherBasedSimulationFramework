
// DPTDoc.h : interface de la classe CMatchStationDoc
//
#pragma once

#include "Basic/HourlyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Simulation/WeatherGradient2.h"
#include "ModelBase/WGInput.h"



class CMatchStationDoc : public CDocument
{
protected: // création à partir de la sérialisation uniquement

	CMatchStationDoc();
	DECLARE_DYNCREATE(CMatchStationDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, LOCATION_INDEX_CHANGE, OUTPUT_CHANGE, CLOSE, LANGUAGE_CHANGE,
		NORMALS_DATABASE_CHANGE, OBSERVATION_DATABASE_CHANGE,
		PROPERTIES_CHANGE, NB_EVENTS
	};


	static const char* DOCUMENT_XML;


	virtual ~CMatchStationDoc();
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
	virtual BOOL IsModified();
	virtual void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint = NULL);


	void SetCurIndex(size_t i, CView* pSender = NULL);
	size_t GetCurIndex()const { return m_curIndex; }
	const WBSF::CLocation& GetLocation(size_t i)const{ ASSERT(m_pLocations.get());  return m_pLocations->at(i); }

	const WBSF::CLocationVectorPtr& GetLocationVector()const{ return m_pLocations; }
	WBSF::CWeatherDatabasePtr& GetNormalsDatabase(){ return m_pNormalsDB; }
	WBSF::CWeatherDatabasePtr& GetObservationDatabase(){ return m_pObservationDB; }
	void SetLocationFilePath(LPCTSTR lpszPathName);
	void SetNormalsFilePath(LPCTSTR lpszPathName);
	void SetObservationFilePath(LPCTSTR lpszPathName);
	void SetWGFilePath(LPCTSTR lpszPathName);

	const WBSF::CWeatherGradient& GetNormalsGradient()const{ return m_gradient; }
	const WBSF::CSearchResultVector& GetNormalsMatch()const{ return m_normalsResult; }
	const WBSF::CSearchResultVector& GetObservationsMatch()const{ return m_observationResult; }
	const WBSF::CNormalsStationVector& GetNormalsStation()const { return m_normalsStations; }
	const WBSF::CWeatherStationVector& GetObservationStation()const { return m_observationStations; }

	int GetYear()const{ return m_year; }
	void SetYear(int in){ if (in != m_year){ m_year = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	size_t GetNbStation()const{ return m_nbStations; }
	void SetNbStation(size_t in){ if (in != m_nbStations){ m_nbStations = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	WBSF::HOURLY_DATA::TVarH GetVariable()const{ return m_variable; }
	void SetVariable(WBSF::HOURLY_DATA::TVarH in){ if (in != m_variable){ m_variable = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }

	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

	void OnInitialUpdate();
	const WBSF::CNormalsStation&	GetNormalsEstimate()const{ return m_normalsEstimate; }

	

#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS



	DECLARE_MESSAGE_MAP()
	afx_msg void OnValidation();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	
	void AdjustDockingLayout(HDWP hdwp);
	

	//properties
	std::string m_outputText;

	//match
	int m_year;
	size_t m_nbStations;
	WBSF::HOURLY_DATA::TVarH m_variable;
		
	std::string m_locationFilePath;
	std::string m_normalsFilePath;
	std::string m_observationFilePath;
	std::string m_WGFilePath;

	//data
	size_t m_curIndex;
	WBSF::CLocationVectorPtr	m_pLocations;
	WBSF::CWeatherDatabasePtr m_pNormalsDB;
	WBSF::CWeatherDatabasePtr m_pObservationDB;


	//result
	WBSF::CWeatherGradient		m_gradient;
	WBSF::CSearchResultVector	m_normalsResult;
	WBSF::CSearchResultVector	m_observationResult;
	WBSF::CNormalsStationVector	m_normalsStations;
	WBSF::CWeatherStationVector	m_observationStations;

	WBSF::CNormalsStation		m_normalsEstimate;


	//optimization
	std::string m_lastLocationFilePath;
	std::string m_lastNormalsFilePath;
	std::string m_lastObservationFilePath;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};

