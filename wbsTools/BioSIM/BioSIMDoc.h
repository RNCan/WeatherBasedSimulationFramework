
// BioSIMDoc.h : interface of the CBioSIMDoc class
//


#pragma once

#include "Simulation/BioSIMProject.h"



class CBioSIMDoc : public CDocument
{
	DECLARE_MESSAGE_MAP()
// Attributes
public:

	
	CBioSIMDoc();
	virtual ~CBioSIMDoc();

	enum TMessage{ INIT = 0, PROJECT_CHANGE, SEL_CHANGE, ID_LAGUAGE_CHANGE };
	static UINT ExecuteTask(void* pParam);


	WBSF::CBioSIMProject& GetProject(){ ASSERT(m_projectPtr.get()); return (WBSF::CBioSIMProject&)(*m_projectPtr.get()); }
	WBSF::CExecutablePtr& GetExecutable(){ return m_projectPtr; }
	WBSF::CProjectStatePtr& GetProjectState(){ return m_projectState; }

	//void Execute();
	ERMsg Execute(CComPtr<ITaskbarList3>& pTaskbarList);
	

	void SetItemExpended(const CString& iName, bool bExpanded);
	void UpdateAllViews(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);
	void OnInitialUpdate(); // called first time after construct

	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void Serialize(CArchive& ar);

	virtual BOOL SaveModified(); // return TRUE if ok to continue
	void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU);

	bool IsInit()const{return m_bInit;}
	bool IsExecute()const{return m_bExecute;}
	void SetIsExecute(bool in){ if (in != m_bExecute){ m_bExecute = in; } }
	const std::string& GetCurSel()const{return m_curSel;}
	void SetCurSel(const std::string& in){ if (in != m_curSel) { m_curSel = in; UpdateAllViews(NULL, SEL_CHANGE); } }
	


protected:

	// create from serialization only
	
	DECLARE_DYNCREATE(CBioSIMDoc)
	
	void LoadProjectState(const CString& filePath);
	void SaveProjectState(const CString& filePath);


	WBSF::CExecutablePtr m_projectPtr;
	WBSF::CBioSIMProject m_lastSaveProject;
	WBSF::CProjectStatePtr m_projectState;
	std::string m_curSel;
	CString m_lastLog;
	bool m_bInit;
	bool m_bExecute;





#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


};
