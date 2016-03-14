
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
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace WBSF;

// CWeatherUpdaterDoc
IMPLEMENT_DYNCREATE(CWeatherUpdaterDoc, CDocument)
BEGIN_MESSAGE_MAP(CWeatherUpdaterDoc, CDocument)
	ON_COMMAND(ID_EXECUTE, OnExecute)
	ON_UPDATE_COMMAND_UI(ID_EXECUTE, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CWeatherUpdaterDoc
CWeatherUpdaterDoc::CWeatherUpdaterDoc()
{
	CAppOption options(_T("Settings"));

	m_currentType = 0;
	m_currentPos.fill(-1);
}

CWeatherUpdaterDoc::~CWeatherUpdaterDoc()
{
}

BOOL CWeatherUpdaterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_currentType = 0;
	m_currentPos.fill(-1);
	m_outputMessage.clear();
	

	return TRUE;
}

// sérialisation de CWeatherUpdaterDoc

BOOL CWeatherUpdaterDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	m_currentType = 0;
	m_currentPos.fill(-1);
	m_outputMessage.clear();

	std::string filePath = CStringA(lpszPathName);
	
//	msg = m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit, dlg.GetCallback());
	
	
	if (msg)
	{
	
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
	
	
	if (m_project != m_lastProject)
	{

		std::string filePath = CStringA(lpszPathName);

		
		//msg = m_pDatabase->Save();
			
		if (!msg)
			UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return (bool)msg;
}

void CWeatherUpdaterDoc::OnCloseDocument()
{
	
	//Save setting
	CAppOption options(_T("Settings"));
	//options.WriteProfileInt(_T("DataTMType"), (int)m_TM.Type() );
	//options.WriteProfileInt(_T("DataStatistic"), (int)m_statistic);
	//options.WriteProfileString(_T("Years"), CString(stdString::to_string(m_years, " ").c_str()));
	//options.WriteProfileString(_T("Filters"), CString(m_filters.to_string().c_str()) );
	//options.WriteProfileString(_T("ChartsPeriod"), CString(m_chartsPeriod.ToString().c_str()));
	//options.WriteProfileInt(_T("ChartsPeriodEnabled"), m_bPeriodEnabled);
	//options.WriteProfileInt(_T("ChartsZoom"), m_chartsZoom);
	
	
	
	
	CDocument::OnCloseDocument();

	
}

BOOL CWeatherUpdaterDoc::IsModified()
{
	
	return m_project != m_lastProject;
}

BOOL CWeatherUpdaterDoc::SaveModified() // return TRUE if ok to continue
{
	
	BOOL bSave = CDocument::SaveModified();
	

	return bSave;
}

#ifdef SHARED_HANDLERS

// Prise en charge des miniatures
void CWeatherUpdaterDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modified ce code pour dessiner les données du document
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
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

// diagnostics pour CWeatherUpdaterDoc

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


void CWeatherUpdaterDoc::SetCurPos(size_t t, size_t p)
{
	ASSERT(t < CTaskBase::NB_TYPES);

	ERMsg msg;

	if (t != m_currentType || p != m_currentPos[t])
	{
		m_currentType = t;
		m_currentPos[t] = p;
		
		UpdateAllViews(NULL, SELECTION_CHANGE, NULL);

		//m_outputMessage[]
		//msg = LoadOutput( GetOUtputName() )
		//if (msg)
			//SetOutputText(SYGetText(msg));

		
	}
}

void CWeatherUpdaterDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_filePath.empty());
}

void CWeatherUpdaterDoc::OnExecute()
{
	ERMsg msg;

	CProgressStepDlg dlg(AfxGetMainWnd());
	dlg.Create();

	//assert(false);//todo
	//msg = m_pDatabase->Open(UtilWin::ToUTF8(lpszPathName), CWeatherDatabase::modeRead, dlg.GetCallback());


	dlg.DestroyWindow();

}

void CWeatherUpdaterDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CWeatherUpdaterDoc::OnInitialUpdate()
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(NULL, NULL, NULL);
}

const std::string& CWeatherUpdaterDoc::GetOutputText(size_t t, size_t p)
{ 
	ASSERT(t < CTaskBase::NB_TYPES);

	static const std::string EMPTY_STRING;
	
	
	if (p < m_project[t].size())
	{
		std::string hash = ToString(t) + "_" + ToString(p) + "_" + m_project[t][p]->Get(CTaskBase::NAME);
		return m_outputMessage[hash];
	}
	
	return EMPTY_STRING;
}

//void CWeatherUpdaterDoc::SetOutputText(size_t t, size_t p, const std::string & in)
//{ 
//	ASSERT(t < CTaskBase::NB_TYPES);
//
//
//	if (in != m_outputText){ m_outputText = in; UpdateAllViews(NULL, OUTPUT_CHANGE, NULL); 
//} 
//
void CWeatherUpdaterDoc::AddTask(size_t t, size_t p, WBSF::CTaskPtr pTask)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(p <= m_project[t].size());

	m_project[t].insert(m_project[t].begin() + p, pTask);

	m_currentType=t;
	m_currentPos[t]=p;

	CDocument::UpdateAllViews(NULL, ADD_TASK, NULL);
}

void CWeatherUpdaterDoc::RemoveTask(size_t t, size_t p)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(p<m_project[t].size());

	m_project[t].erase(m_project[t].begin() + p);

	m_currentType = t;
	m_currentPos[t] = p>=m_project[t].size() ? p - 1:p;


	CDocument::UpdateAllViews(NULL, REMOVE_TASK, NULL);
}

void CWeatherUpdaterDoc::Move(size_t t, size_t from, size_t to, bool bAfter)
{
	ASSERT(t < CTaskBase::NB_TYPES);
	ASSERT(from<m_project[t].size());
	ASSERT(to<m_project[t].size());
	ASSERT(from != to);
	ASSERT(from!=NOT_INIT);
	ASSERT(to != NOT_INIT);

	if (from == to)
		return;
	
	if (to < from)
	{
		if (bAfter)
			to++;
	}
	else//if (to > from)
	{
		if (!bAfter)
			to--;
	}
		
	ASSERT(to != NOT_INIT);

	CTaskPtr pTask = m_project[t][from];
	m_project[t].erase(m_project[t].begin() + from);
	m_project[t].insert(m_project[t].begin() + to, pTask);


	m_currentType = t;
	m_currentPos[t] = to;

	CDocument::UpdateAllViews(NULL, SELECTION_CHANGE, NULL);
}
