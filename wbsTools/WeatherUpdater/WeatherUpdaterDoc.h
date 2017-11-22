#pragma once


#include "Basic/NormalsDatabase.h"
#include "Tasks/TaskBase.h"
#include "Tasks/TaskFactory.h"


class CWeatherUpdaterDoc : public CDocument
{
protected:

	CWeatherUpdaterDoc();
	DECLARE_DYNCREATE(CWeatherUpdaterDoc)

	// Attributs
public:

	//
	enum TEvent
	{
		INIT = 0, ADD_TASK, REMOVE_TASK, SELECTION_WILL_CHANGE, SELECTION_CHANGE, TASK_CHANGE, LANGUAGE_CHANGE, NB_EVENTS
	};

	size_t GetCurT()const { return m_currentType; }
	//size_t GetCurA(size_t t, size_t p)const { return m_currentAttribute; }

	size_t GetCurP(size_t t)const { return m_currentTask[t]; }
	void SetCurP(size_t t, size_t p);//, size_t a=-1
	void SetLanguage(size_t ID);
	
	

	const WBSF::CTaskPtrVector& GetTaskVector(size_t t)const{ return m_project[t]; }
	WBSF::CTaskPtr GetTask(size_t t, size_t p){ ASSERT(t < WBSF::CTaskBase::NB_TYPES);  return p < m_project[t].size() ? m_project[t][p] : WBSF::CTaskPtr(); }
	
	void InsertTask(size_t t, size_t p, WBSF::CTaskPtr& pTask);
	void RemoveTask(size_t t, size_t p);
	void Move(size_t t, size_t from, size_t to, bool bAfter = true);
	
	const std::string& GetOutputText(size_t t, size_t p);

	void OnInitialUpdate();

	virtual ~CWeatherUpdaterDoc();
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
	virtual BOOL IsModified();
	virtual void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint=NULL);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU);

	std::string GetUpdaterList()const;
	const std::string& GetFilePath()const{return m_filePath; }

	bool IsExecute()const{ return m_bExecute;  }

	WBSF::CTasksProject& GetProject(){ return m_project; }


protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnExecute();
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	

	size_t m_currentType;
	//size_t m_currentAttribute;
	std::array<size_t, WBSF::CTaskBase::NB_TYPES> m_currentTask;

	std::string m_filePath;
	WBSF::CTasksProject m_project;
	WBSF::CTasksProject m_lastProject;
	
	std::array<std::map<std::string, std::string>, WBSF::CTaskBase::NB_TYPES> m_outputMessage;
	std::string m_lastLog;

	bool m_bExecute;
	
	
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
	// Fonction d'assistance qui définit le contenu de recherche pour un gestionnaire de recherche
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

};
