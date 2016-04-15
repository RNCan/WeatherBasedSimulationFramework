
// DPTDoc.cpp : implémentation de la classe CWeatherUpdaterDoc
//

#include "stdafx.h"

// SHARED_HANDLERS peuvent être définis dans les gestionnaires d'aperçu, de miniature
// et de recherche d'implémentation de study ATL et permettent la partage de code de document avec ce study.
#ifndef SHARED_HANDLERS
#include "WeatherUpdater.h"
#endif


#include "WeatherUpdaterDoc.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"
#include "Tasks/TaskFactory.h"
#include "MainFrm.h"
#include "OutputView.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace WBSF;
using namespace UtilWin;



namespace zen
{
	void writeStruc(const std::array<std::map<std::string, std::string>, WBSF::CTaskBase::NB_TYPES>& in, zen::XmlElement& output)
	{
		size_t t = 0;
		std::for_each(in.begin(), in.end(),
			[&](const std::map<std::string, std::string> & childVal)
		{
			if (!childVal.empty())
			{
				zen::XmlElement& newChild = output.addChild("Tasks");
				newChild.setAttribute("type", std::to_string(t));
				std::for_each(childVal.begin(), childVal.end(),
					[&](const std::pair<std::string, std::string> & childVal2)
				{
					zen::XmlElement& newChild2 = newChild.addChild("Task");
					newChild2.setAttribute("name", childVal2.first);
					newChild2.setValue(childVal2.second);
				}
				);

				t++;
			}
		}
		);
	}

	
	bool readStruc(const zen::XmlElement& input, std::array<std::map<std::string, std::string>, WBSF::CTaskBase::NB_TYPES>& out)
	{
		bool success = false;
		out.fill(std::map<std::string, std::string>());
		
		auto iterPair = input.getChildren("Tasks");
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			std::string str;
			if (iter->getAttribute("type", str))
			{
				size_t t = ToValue<size_t>(str);
				if (t < out.size())
				{
					auto iterPair2 = iter->getChildren("Task");
					for (auto it = iterPair2.first; it != iterPair2.second; ++it)
					{
						std::string name;
						std::string msg;
						if (it->getAttribute("name", name) && it->getValue(msg))
						{
							out[t][name] = msg;
							success = true;
						}
					}
				}
			}
		}

		return success;
	}
}
//
//template <typename U, class T = std::vector<U>> inline
//bool readStruc4(const zen::XmlElement& input, T& out, const char* XMLFlag)
//{
//	bool success = true;
//	out.clear();
//
//	auto iterPair = input.getChildren(XMLFlag);
//	out.resize(std::distance(iterPair.first, iterPair.second));
//	T::iterator it = out.begin();
//	for (auto iter = iterPair.first; iter != iterPair.second; ++iter, it++)
//	{
//		if (!zen::readStruc(*iter, *it))
//			success = false;
//	}
//	return success;
//}

// CWeatherUpdaterDoc
IMPLEMENT_DYNCREATE(CWeatherUpdaterDoc, CDocument)
BEGIN_MESSAGE_MAP(CWeatherUpdaterDoc, CDocument)
	ON_COMMAND(ID_EXECUTE, OnExecute)
	ON_UPDATE_COMMAND_UI(ID_EXECUTE, OnUpdateToolbar)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CWeatherUpdaterDoc
CWeatherUpdaterDoc::CWeatherUpdaterDoc()
{
	CAppOption options(_T("Settings"));

	m_currentType = 0;
	m_currentTask.fill(-1);
	m_bExecute = false;
}

CWeatherUpdaterDoc::~CWeatherUpdaterDoc()
{
}

BOOL CWeatherUpdaterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_currentType = 0;
	m_currentTask.fill(-1);
	m_outputMessage.fill(std::map<std::string, std::string>());
	m_filePath.clear();
	m_project.clear();
	m_lastProject.clear();

	return TRUE;
}

// sérialisation de CWeatherUpdaterDoc

BOOL CWeatherUpdaterDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	m_currentType = 0;
	m_currentTask.fill(-1);
	m_outputMessage.fill(std::map<std::string, std::string>());
	m_project.clear();
	m_lastProject.clear();

	std::string filePath = CStringA(lpszPathName);
	
	
	msg = m_project.Load(filePath);
	
	
	if (msg)
	{
		m_lastProject = m_project;
		m_filePath = filePath;

		SetFileExtension(filePath, ".txt");
		zen::LoadXML(filePath, "OutputMessage", "1", m_outputMessage);

	}
	else
	{
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return (bool)msg;
}

BOOL CWeatherUpdaterDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;
	
	
	if (m_project.empty() || m_project != m_lastProject)
	{
		std::string filePath = CStringA(lpszPathName);
		
		msg = m_project.Save(filePath);
			
		if (msg)
		{
			m_filePath = filePath;
			m_lastProject = m_project;
		}
		else
		{
			UtilWin::SYShowMessage(msg, AfxGetMainWnd());
		}
	}

	return (bool)msg;
}

void CWeatherUpdaterDoc::OnCloseDocument()
{
	CDocument::OnCloseDocument();
}

BOOL CWeatherUpdaterDoc::IsModified()
{
	
	return m_project != m_lastProject;
}

BOOL CWeatherUpdaterDoc::SaveModified() // return TRUE if ok to continue
{
	if (m_bExecute)
		return FALSE;

	BOOL bSave = CDocument::SaveModified();

	return bSave;
}
// diagnostics pour CWeatherUpdaterDoc


void CWeatherUpdaterDoc::SetCurP(size_t t, size_t p)//, size_t a
{
	ASSERT(t < CTaskBase::NB_TYPES);

	ERMsg msg;

	if (t != m_currentType || p != m_currentTask[t] )
	{
		for (size_t tt = 0; tt < m_currentTask.size(); tt++)
			m_currentTask[tt] = NOT_INIT;

		m_currentType = t;
		m_currentTask[t] = p;
		
		UpdateAllViews(NULL, SELECTION_CHANGE, NULL);
	}

//	m_currentAttribute = a;
}

std::string CWeatherUpdaterDoc::GetUpdaterList()const
{
	std::string str;
	for (int i = 0; i < m_project[CTaskBase::UPDATER].size(); i++)
	{
		//add empty element
		str += "|" + m_project[CTaskBase::UPDATER][i]->m_name;//que faire si plusieur foisle mem nom???
	}

	return str;
}




void CWeatherUpdaterDoc::OnExecute()
{
	ERMsg msg;

	if (!m_bExecute)
	{
		CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
		COutputView* pView = (COutputView*)pMainFrm->GetActiveView();
		CProgressWnd& progressWnd = pView->GetProgressWnd();


		m_bExecute = true;
		pView->AdjustLayout();//open the progress window

		progressWnd.SetTaskbarList(pMainFrm->GetTaskbarList());
		CProgressStepDlgParam param(&m_project);

		msg = progressWnd.Execute(CWeatherUpdaterApp::ExecuteTasks, &param);
		m_lastLog = GetOutputString(msg, progressWnd.GetCallback(), true);

		
		ReplaceString(m_lastLog, "\n", "|");
		ReplaceString(m_lastLog, "\r", "");
		
		m_bExecute = false;
		pView->AdjustLayout();
	
		//transfer message 
		for (size_t t = 0; t < m_project.size(); t++)
		{
			for (size_t p = 0; p < m_project[t].size(); p++)
			{
				if (m_project[t][p]->m_bExecute)
					m_outputMessage[t][m_project[t][p]->m_name] = m_project[t][p]->GetLastMsg();
			}
		}

		//save message
		std::string filePath(m_filePath);
		SetFileExtension(filePath, ".txt");
		msg = zen::SaveXML(filePath, "OutputMessage", "1", m_outputMessage);

		//if (pActivePane)
			//pActivePane->ShowPane(true, true, true);

		UpdateAllViews(NULL, TASK_CHANGE, NULL);
	}
}

void CWeatherUpdaterDoc::SetLanguage(size_t ID)
{
	for (size_t t = 0; t < m_project.size(); t++)
		for (size_t p = 0; p < m_project[t].size(); p++)
			m_project[t][p]->UpdateLanguage();

	UpdateAllViews(NULL, CWeatherUpdaterDoc::LANGUAGE_CHANGE);
}

void CWeatherUpdaterDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CWeatherUpdaterDoc::OnInitialUpdate()
{
	UpdateAllViews(NULL, INIT, NULL);
}

const std::string& CWeatherUpdaterDoc::GetOutputText(size_t t, size_t p)
{ 
	ASSERT(t < CTaskBase::NB_TYPES);
	
	if (p == NOT_INIT)
		return m_lastLog;
	
	ASSERT(p < m_project[t].size());
	return m_outputMessage[t][m_project[t][p]->m_name];
}

//void CWeatherUpdaterDoc::SetOutputText(size_t t, size_t p, const std::string & in)
//{ 
//	ASSERT(t < CTaskBase::NB_TYPES);
//
//
//	if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); 
//} 
//
void CWeatherUpdaterDoc::InsertTask(size_t t, size_t p, WBSF::CTaskPtr& pTask)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(p <= m_project[t].size());

	pTask->SetProject(&m_project);
	m_project[t].insert(m_project[t].begin() + p, pTask);

	CDocument::UpdateAllViews(NULL, ADD_TASK, NULL);
}

void CWeatherUpdaterDoc::RemoveTask(size_t t, size_t p)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(p<m_project[t].size());

	std::map<std::string, std::string>::iterator it = m_outputMessage[t].find(m_project[t][p]->m_name);
	if (it != m_outputMessage[t].end())
		m_outputMessage[t].erase(it);

	m_project[t].erase(m_project[t].begin() + p);
	

	CDocument::UpdateAllViews(NULL, REMOVE_TASK, NULL);
}

void CWeatherUpdaterDoc::Move(size_t t, size_t from, size_t to, bool /*bAfter*/)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(from<m_project[t].size());
	ASSERT((to == NOT_INIT) || to<m_project[t].size());
	ASSERT(from != to);
	ASSERT(from!=NOT_INIT);

	if (from == to)
		return;
	
	if (to == NOT_INIT || to < from)
		to++;
		
	ASSERT(to != NOT_INIT);
	ASSERT(to<m_project[t].size());

	CTaskPtr pTask = m_project[t][from];
	m_project[t].erase(m_project[t].begin() + from);
	m_project[t].insert(m_project[t].begin() + to, pTask);


	m_currentType = t;
	m_currentTask[t] = to;

	CDocument::UpdateAllViews(NULL, SELECTION_CHANGE, NULL);
}

void CWeatherUpdaterDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_EXECUTE:	pCmdUI->Enable(!m_filePath.empty() && !m_bExecute); break;
	case ID_FILE_SAVE:	pCmdUI->Enable(true); break;
	}

}

void CWeatherUpdaterDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU);
	SetTitle(GetPathName());
}


#ifdef _DEBUG
void CWeatherUpdaterDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWeatherUpdaterDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


#ifdef SHARED_HANDLERS

// Prise en charge des miniatures
void CWeatherUpdaterDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modified ce code pour dessiner les données du document
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support pour les gestionnaires de recherche
void CWeatherUpdaterDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Définir le contenu de recherche à partir des données du document. 
	// Les parties du contenu doivent être séparées par ";"

	// Par exemple :  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CWeatherUpdaterDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

