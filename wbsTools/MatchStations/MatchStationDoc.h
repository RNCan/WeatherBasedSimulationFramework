
// DPTDoc.h : interface de la classe CMatchStationDoc
//
#pragma once

#include "Basic/HourlyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Simulation/WeatherGradient.h"
#include "ModelBase/WGInput.h"

class COutputView;
class CProgressWnd;

class CMatchStationDoc : public CDocument
{

	DECLARE_DYNCREATE(CMatchStationDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, LOCATION_INDEX_CHANGE, OUTPUT_CHANGE, LANGUAGE_CHANGE,
		NORMALS_DATABASE_CHANGE, OBSERVATION_DATABASE_CHANGE,
		PROPERTIES_CHANGE, NB_EVENTS
	};

	
	enum TObservation{ T_HOURLY, T_DAILY, T_NORMALS, T_LOCATION };


	static const char* DOCUMENT_XML;


	CMatchStationDoc();
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

	const WBSF::CLocationVectorPtr& GetLocations()const{ return m_pLocations; }
	WBSF::CWeatherDatabasePtr& GetNormalsDatabase(){ return m_pNormalsDB; }
	WBSF::CWeatherDatabasePtr& GetObservationDatabase(){ return m_obsType == T_HOURLY ? m_pHourlyDB : m_pDailyDB; }
	WBSF::CWeatherDatabasePtr& GetDailyDatabase(){ return m_pDailyDB; }
	WBSF::CWeatherDatabasePtr& GetHourlyDatabase(){ return m_pHourlyDB; }

	
	void SetLocationFilePath(const std::string& filepath);
	void SetNormalsFilePath(const std::string& filepath);
	void SetDailyFilePath(const std::string& filepath);
	void SetHourlyFilePath(const std::string& filepath);
	void SetWGFilePath(const std::string& filepath);

	const WBSF::CWeatherGradient& GetNormalsGradient()const{ return m_gradient; }
	const WBSF::CSearchResultVector& GetNormalsMatch()const{ return m_normalsResult; }
	const WBSF::CSearchResultVector& GetObservationsMatch()const{ return m_obsType == T_HOURLY ? m_hourlyResult : m_dailyResult; }
	const WBSF::CNormalsStationVector& GetNormalsStation()const { return m_normalsStations; }
	const WBSF::CWeatherStationVector& GetObservationStation()const { return m_obsType == T_HOURLY ? m_hourlyStations : m_dailyStations; }

	int GetYear()const{ return m_year; }
	void SetYear(int in){ if (in != m_year){ m_year = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	size_t GetNbStation()const{ return m_nbStations; }
	void SetNbStation(size_t in){ if (in != m_nbStations){ m_nbStations = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	WBSF::HOURLY_DATA::TVarH GetVariable()const{ return m_variable; }
	void SetVariable(WBSF::HOURLY_DATA::TVarH in){ if (in != m_variable){ m_variable = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	size_t GetObservationType(){ return m_obsType; }
	void SetObservationType(size_t in){ if (in != m_obsType){ m_obsType = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
	


	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

	void OnInitialUpdate();
	CProgressWnd& GetProgressWnd(COutputView* pView);
	COutputView* GetOutpoutView();
	bool IsExecute()const{ return m_bExecute; }
	const WBSF::CNormalsStation&	GetNormalsEstimate()const{ return m_normalsEstimate; }

protected:


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
	std::string m_dailyFilePath;
	std::string m_hourlyFilePath;

	//data
	size_t						m_curIndex;
	WBSF::CLocationVectorPtr	m_pLocations;
	WBSF::CWeatherDatabasePtr	m_pNormalsDB;
	WBSF::CWeatherDatabasePtr	m_pDailyDB;
	WBSF::CWeatherDatabasePtr	m_pHourlyDB;
	size_t						m_obsType;
	

	//result
	WBSF::CWeatherGradient		m_gradient;
	WBSF::CSearchResultVector	m_normalsResult;
	WBSF::CSearchResultVector	m_dailyResult;
	WBSF::CSearchResultVector	m_hourlyResult;
	WBSF::CNormalsStationVector	m_normalsStations;
	WBSF::CWeatherStationVector	m_dailyStations;
	WBSF::CWeatherStationVector	m_hourlyStations;

	WBSF::CNormalsStation		m_normalsEstimate;


	//optimization
	std::string m_lastLocationFilePath;
	std::string m_lastNormalsFilePath;
	std::string m_lastDailyFilePath;
	std::string m_lastHourlyFilePath;
	bool m_bExecute;


	static UINT CMatchStationDoc::Execute(void* pParam);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS


};

