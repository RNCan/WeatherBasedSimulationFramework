#pragma once


#include "Basic/NormalsDatabase.h"
#include "wbs/TaskBase.h"



typedef std::vector<WBSF::CTaskPtr> CTaskPtrVector;
typedef std::array<CTaskPtrVector, WBSF::CTaskBase::NB_TYPES> CWUProject;

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
		INIT = 0, ADD_TASK, REMOVE_TASK, SELECTION_CHANGE, TASK_CHANGE, LANGUAGE_CHANGE, NB_EVENTS
	};


	
//	void SetCurType(size_t t);
	size_t GetCurType()const { return m_currentType; }
	void SetCurPos(size_t t, size_t p);
	size_t GetCurPos(size_t t)const { return m_currentPos[t]; }

	//void SetTask(size_t t, size_t p, const CTaskPtr& task);
	const CTaskPtrVector& GetTaskVector(size_t t)const{ return m_project[t]; }
	WBSF::CTaskPtr GetTask(size_t t, size_t p){ ASSERT(t < WBSF::CTaskBase::NB_TYPES);  return p < m_project[t].size() ? m_project[t][p] : WBSF::CTaskPtr(); }
	
	void AddTask(size_t t, size_t p, WBSF::CTaskPtr pTask);
	void RemoveTask(size_t t, size_t p);
	void Move(size_t t, size_t from, size_t to, bool bAfter = true);

	//void SetTask(size_t t, const std::string& ID, const std::string& task);
	//const CTask& GetTask(size_t t, const std::string& ID)const{ return m_task; }

	//const WBSF::CNormalsStationPtr& GetCurTask()const{ return m_pStation; }
	//void SetCurTask(WBSF::CLocation& station, CView* pSender = NULL);

	const std::string& GetOutputText(size_t t, size_t p);
	
	//void SetOutputText(size_t t, size_t p, const std::string & in);
	

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
	//std::string m_outputText;

	size_t m_currentType;
	std::array<size_t, WBSF::CTaskBase::NB_TYPES> m_currentPos;

	std::string m_filePath;
	CWUProject m_project;
	CWUProject m_lastProject;
	//size_t m_lastCurrentType;

	//CWUProject m_task;
	std::map<std::string, std::string> m_outputMessage;

};
