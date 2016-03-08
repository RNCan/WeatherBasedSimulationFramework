
// DPTDoc.h : interface de la classe CMatchStationDoc
//
#pragma once

#include "Basic/HourlyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Simulation/WeatherGradient2.h"


namespace WBSF
{

	typedef std::array<CNormalsStationVector, HOURLY_DATA::NB_VAR_H> CNormalsArray;
	typedef std::array<CWeatherStationVector, HOURLY_DATA::NB_VAR_H> CObservationArray;

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


		void SetCurIndex(size_t i, CView* pSender = NULL);
		size_t GetCurIndex()const { return m_curIndex; }
		const CLocation& GetLocation(size_t i)const{ ASSERT(m_pLocations.get());  return m_pLocations->at(i); }

		const CLocationVectorPtr& GetLocationVector()const{ return m_pLocations; }
		CWeatherDatabasePtr& GetNormalsDatabase(){ return m_pNormalsDB; }
		CWeatherDatabasePtr& GetObservationDatabase(){ return m_pObservationDB; }
		void SetNormalsDatabase(LPCTSTR lpszPathName);
		void SetObservationDatabase(LPCTSTR lpszPathName);

		const CWeatherGradient& GetNormalsGradient()const{ return m_gradient; }
		const CSearchResultVector& GetNormalsMatch()const{ return m_normalsResult; }
		const CSearchResultVector& GetObservationsMatch()const{ return m_observationResult; }
		const CNormalsStationVector& GetNormalsStation()const { return m_normalsStations; }
		const CWeatherStationVector& GetObservationStation()const { return m_observationStations; }

		//const CSearchArray& GetNormalsMatch()const{ return m_normalsResult; }
		//const CSearchArray& GetObservationsMatch()const{ return m_observationResult; }
		//const CNormalsArray&  GetNormalsStation()const { return m_normalsStations; }
		//const CObservationArray&  GetObservationStation()const { return m_observationStations; }

		int GetYear()const{ return m_year; }
		void SetYear(int in){ if (in != m_year){ m_year = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
		size_t GetNbStation()const{ return m_nbStation; }
		void SetNbStation(size_t in){ if (in != m_nbStation){ m_nbStation = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
		//size_t GetNbObservationStation()const{ return m_nbObservationStation; }
		//void SetNbObservationStation(size_t in){ if (in != m_nbObservationStation){ m_nbObservationStation = in; UpdateAllViews(NULL, OBSERVATION_PROPERTIES_CHANGE); } }
		//CWVariables GetVariables()const{ return m_variables; }
		//void SetVariables(CWVariables in){ if (in != m_variables){ m_variables = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }
		HOURLY_DATA::TVarH GetVariable()const{ return m_variable; }
		void SetVariable(HOURLY_DATA::TVarH in){ if (in != m_variable){ m_variable = in; UpdateAllViews(NULL, PROPERTIES_CHANGE); } }

		const std::string& GetOutputText()const{ return m_outputText; }
		void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

		void OnInitialUpdate();
		//int GetChartsZoom()const{ return m_chartsZoom; }
		//void SetChartsZoom(int in){ m_chartsZoom = in; UpdateAllViews(NULL, ZOOM_CHANGE, NULL); }

		const CNormalsStation&	GetNormalsEstimate()const{ return m_normalsEstimate; }
		// Opérations
	public:

		// Substitutions
	public:
		
		//virtual void UpdateFrameCounts();
		//virtual void ActivateFrame(int nCmdShow = -1);
		virtual BOOL OnNewDocument();
		virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
		virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
		virtual void OnCloseDocument();
		virtual BOOL SaveModified(); // return TRUE if ok to continue
		virtual BOOL IsModified();
		virtual void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint = NULL);

#ifdef SHARED_HANDLERS
		virtual void InitializeSearchContent();
		virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

		// Implémentation
	public:
		virtual ~CMatchStationDoc();
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
		void AdjustDockingLayout(HDWP hdwp);
		//ERMsg OpenDatabase(LPCTSTR lpszPathName);

	protected:


		//properties
		std::string m_outputText;

		//match
		int m_year;
		//CWVariables m_variables;
		HOURLY_DATA::TVarH m_variable;
		size_t m_nbStation;
		std::string m_normalsFilePath;
		std::string m_observationFilePath;

		//data
		size_t m_curIndex;
		CLocationVectorPtr	m_pLocations;
		CWeatherDatabasePtr m_pNormalsDB;
		CWeatherDatabasePtr m_pObservationDB;


		//result
		CWeatherGradient		m_gradient;
		CSearchResultVector		m_normalsResult;
		CSearchResultVector		m_observationResult;
		CNormalsStationVector	m_normalsStations;
		CWeatherStationVector	m_observationStations;

		CNormalsStation			m_normalsEstimate;
	};
}