
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

	m_currentTaskID.clear(); 
}

CWeatherUpdaterDoc::~CWeatherUpdaterDoc()
{
}

BOOL CWeatherUpdaterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_currentTaskID.clear();
	m_outputText.clear();
	

	return TRUE;
}

// sérialisation de CWeatherUpdaterDoc

BOOL CWeatherUpdaterDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	m_currentTaskID.clear();
	m_outputText.clear();

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


void CWeatherUpdaterDoc::SetCurTaskID(const std::string& ID)
{
	ERMsg msg;

	if (ID != m_currentTaskID)
	{
		m_currentTaskID = ID;


		if (!ID.empty())
		{
			m_task;
		}
		
		UpdateAllViews(NULL, SELECTION_CHANGE, NULL);

		if (!msg)
			SetOutputText(SYGetText(msg));
	}
}


void CWeatherUpdaterDoc::SetTask(const std::string& ID, const CTask& task)
{
	m_task = task;
	UpdateAllViews(NULL, TASK_CHANGE, NULL);
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


	SetOutputText("La vérification s'affichera ici un jour...");

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

