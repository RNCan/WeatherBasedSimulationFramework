#pragma once


#include "Basic/NormalsDatabase.h"




typedef std::string CTask;

class CWeatherUpdaterDoc : public CDocument
{
protected: // création à partir de la sérialisation uniquement

	CWeatherUpdaterDoc();
	DECLARE_DYNCREATE(CWeatherUpdaterDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, SELECTION_CHANGE, TASK_CHANGE, OUTPUT_CHANGE, LANGUAGE_CHANGE, NB_EVENTS
	};



	void SetCurTaskID(const std::string& ID);
	const std::string& GetCurTaskID()const { return m_currentTaskID; }

	void SetTask(const std::string& ID, const std::string& task);
	const CTask& GetTask(const std::string& ID)const{ return m_task;}

	//const WBSF::CNormalsStationPtr& GetCurTask()const{ return m_pStation; }
	//void SetCurTask(WBSF::CLocation& station, CView* pSender = NULL);

	const std::string& GetOutputText()const{ return m_outputText; }
	void SetOutputText(const std::string & in){ if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); } }

	void OnInitialUpdate();

	// Opérations
public:

	// Substitutions
public:

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
	virtual ~CWeatherUpdaterDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


#ifdef SHARED_HANDLERS
	// Fonction d'assistance qui définit le contenu de recherche pour un gestionnaire de recherche
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS


	DECLARE_MESSAGE_MAP()
	afx_msg void OnExecute();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);


	//properties
	std::string m_outputText;
	std::string m_currentTaskID;

	std::string m_filePath;
	std::string m_project;
	std::string m_lastProject;

	CTask m_task;


};
