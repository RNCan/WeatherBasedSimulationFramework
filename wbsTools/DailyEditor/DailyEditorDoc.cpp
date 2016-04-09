
// DPTDoc.cpp : implémentation de la classe CDailyEditorDoc
//

#include "stdafx.h"

// SHARED_HANDLERS peuvent être définis dans les gestionnaires d'aperçu, de miniature
// et de recherche d'implémentation de study ATL et permettent la partage de code de document avec ce study.
#ifndef SHARED_HANDLERS
#include "DailyEditor.h"
#endif

#include <propkey.h>
#include "DailyEditorDoc.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"
#include "Basic/UtilStd.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace WBSF;

// CDailyEditorDoc
IMPLEMENT_DYNCREATE(CDailyEditorDoc, CDocument)
BEGIN_MESSAGE_MAP(CDailyEditorDoc, CDocument)
	ON_COMMAND(ID_VALIDATION, OnValidation)
	ON_UPDATE_COMMAND_UI(ID_VALIDATION, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CDailyEditorDoc
CDailyEditorDoc::CDailyEditorDoc()
{
	CAppOption options(_T("Settings"));

	m_bDataInEdition = false;
	m_stationIndex=UNKNOWN_POS; 
	//m_bForAllYears = false;

	m_statistic = options.GetProfileInt(_T("DataStatistic"), MEAN);
	m_TM = CTM(options.GetProfileInt(_T("DataTMType"), CTM::DAILY));
	m_filters = CStringA(options.GetProfileString(_T("Filters")));
	std::string tmp = CStringA(options.GetProfileString(_T("ChartsPeriod")));
	m_chartsPeriod.FromString(tmp);
	m_bPeriodEnabled = options.GetProfileInt(_T("ChartsPeriodEnabled"), 0);
	m_chartsZoom = options.GetProfileInt(_T("ChartsZoom"), 0);


	std::string str = CStringA(options.GetProfileString(_T("Years")));
	m_years = WBSF::to_object<int, std::set<int>>(str, " ");
}

CDailyEditorDoc::~CDailyEditorDoc()
{
}

BOOL CDailyEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_pStation.reset();
	m_modifiedStation.clear();
	m_outputText.clear();

	m_pDatabase.reset(new CDailyDatabase);
	m_pStation.reset(new CWeatherStation);

	return TRUE;
}

// sérialisation de CDailyEditorDoc

BOOL CDailyEditorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_pDatabase.reset(new CDailyDatabase);
	m_pStation.reset(new CWeatherStation);
	m_modifiedStation.clear();
	m_outputText.clear();

	std::string filePath = CStringA(lpszPathName);

	
	CProgressStepDlg dlg(AfxGetMainWnd() );
	dlg.Create();

	msg = m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit, dlg.GetCallback());
	dlg.DestroyWindow();

	
	if (msg)
	{
		//not init by default
		const std::set<int>& years = m_pDatabase->GetYears();
		if (!m_chartsPeriod.IsInit() && !years.empty())
			m_chartsPeriod = CTPeriod(CTRef(*years.begin(), FIRST_MONTH, FIRST_DAY), CTRef(*years.rbegin(), LAST_MONTH, LAST_DAY));
	}
	else
	{
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return (bool)msg;
}

BOOL CDailyEditorDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;
	if (!m_modifiedStation.empty() ||
		!UtilWin::FileExist(lpszPathName) )
	{

		std::string filePath = CStringA(lpszPathName);
		
		if ( !m_pDatabase->IsOpen() )//create a new database
			msg = m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit);

		if (msg)
			msg = m_pDatabase->Save();
			
		if (!msg)
			UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return (bool)msg;
}

void CDailyEditorDoc::OnCloseDocument()
{
	UpdateAllViews(NULL, CDailyEditorDoc::CLOSE, NULL);

	
	//Save setting
	CAppOption options(_T("Settings"));
	options.WriteProfileInt(_T("DataTMType"), (int)m_TM.Type() );
	options.WriteProfileInt(_T("DataStatistic"), (int)m_statistic);
	options.WriteProfileString(_T("Years"), CString(WBSF::to_string(m_years, " ").c_str()));
	options.WriteProfileString(_T("Filters"), CString(m_filters.to_string().c_str()) );
	options.WriteProfileString(_T("ChartsPeriod"), CString(m_chartsPeriod.ToString().c_str()));
	options.WriteProfileInt(_T("ChartsPeriodEnabled"), m_bPeriodEnabled);
	options.WriteProfileInt(_T("ChartsZoom"), m_chartsZoom);
	
	ASSERT(!m_bDataInEdition);
	
	ERMsg msg = m_pDatabase->Close(false);
	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	
	CDocument::OnCloseDocument();

	
}

BOOL CDailyEditorDoc::IsModified()
{
	if (m_pDatabase == NULL)
		return FALSE;

	return !m_modifiedStation.empty();
}

BOOL CDailyEditorDoc::SaveModified() // return TRUE if ok to continue
{
	if (m_bDataInEdition)
		return FALSE;

	BOOL bSave = CDocument::SaveModified();
	if (bSave)
		m_modifiedStation.clear();

	return bSave;
}

#ifdef SHARED_HANDLERS

// Prise en charge des miniatures
void CDailyEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CDailyEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Définir le contenu de recherche à partir des données du document. 
	// Les parties du contenu doivent être séparées par ";"

	// Par exemple :  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CDailyEditorDoc::SetSearchContent(const CString& value)
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

// diagnostics pour CDailyEditorDoc

#ifdef _DEBUG
void CDailyEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDailyEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


void CDailyEditorDoc::SetCurStationIndex(size_t i, CView* pSender)
{
	ERMsg msg;

	if (i != m_stationIndex)
	{
		m_pStation->Reset();
		m_stationIndex = i;


		if (i != UNKNOWN_POS)
		{
			CWaitCursor wait;
			assert(i < m_pDatabase->size());
			msg = m_pDatabase->Get(*m_pStation, i);
			assert(m_pStation->IsInit());
		}
		
		UpdateAllViews(pSender, STATION_INDEX_CHANGE, NULL);

		if (!msg)
			SetOutputText(WBSF::GetText(msg));
	}

}


bool CDailyEditorDoc::CancelDataEdition()
{
	ERMsg msg;

	ASSERT(m_stationIndex != UNKNOWN_POS);
	ASSERT(m_stationIndex < m_pDatabase->size());
	msg = m_pDatabase->Get(*m_pStation, m_stationIndex);
	ASSERT(m_pStation->IsInit());

	if (!msg)
		SetOutputText(WBSF::GetText(msg));
		

	return msg;
}


void CDailyEditorDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CDailyEditorDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDatabase->IsOpen() && !m_bDataInEdition);

}

void CDailyEditorDoc::OnValidation()
{
	ERMsg msg;

	CProgressStepDlg dlg(AfxGetMainWnd());
	dlg.Create();

	//assert(false);//todo
	//msg = m_pDatabase->Open(UtilWin::ToUTF8(lpszPathName), CWeatherDatabase::modeRead, dlg.GetCallback());
	dlg.DestroyWindow();


	SetOutputText("La vérification s'affichera ici un jour...");

}

void CDailyEditorDoc::SetCurStation(CLocation& location, CView* pSender)
{
	ASSERT(m_pDatabase->IsOpen());
	ASSERT(m_pStation);
	ASSERT(m_stationIndex<m_pDatabase->size());

	CLocation& actualLocation = ((CLocation&)(*m_pStation));
	if (location != actualLocation )
	{ 
		actualLocation = location;
		m_pDatabase->Set(m_stationIndex, location);//update coordinate station info

		m_modifiedStation.insert(m_stationIndex);
		UpdateAllViews(pSender, LOCATION_CHANGE);
		SetModifiedFlag();
	}
	
}

bool CDailyEditorDoc::IsStationModified(size_t stationIndex)const
{
	return m_modifiedStation.find(stationIndex) != m_modifiedStation.end();
}

void CDailyEditorDoc::InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc, BOOL bMakeVisible)
{
	CMainFrame* pMainFrm = (CMainFrame*)pFrame;
	pMainFrm->InitialUpdateFrame(pDoc, bMakeVisible);
}

