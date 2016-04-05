
// DPTDoc.cpp : implémentation de la classe CHourlyEditorDoc
//

#include "stdafx.h"

// SHARED_HANDLERS peuvent être définis dans les gestionnaires d'aperçu, de miniature
// et de recherche d'implémentation de study ATL et permettent la partage de code de document avec ce study.
#ifndef SHARED_HANDLERS
#include "HourlyEditor.h"
#endif

#include <propkey.h>

#include "HourlyEditorDoc.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace WBSF;


// CHourlyEditorDoc
IMPLEMENT_DYNCREATE(CHourlyEditorDoc, CDocument)
BEGIN_MESSAGE_MAP(CHourlyEditorDoc, CDocument)
	ON_COMMAND(ID_VALIDATION, OnValidation)
	ON_UPDATE_COMMAND_UI(ID_VALIDATION, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CHourlyEditorDoc
CHourlyEditorDoc::CHourlyEditorDoc()
{
	CAppOption options(_T("Settings"));

	m_bDataInEdition = false;
	m_stationIndex=UNKNOWN_POS;
	//m_bForAllYears = false;

	m_statistic = options.GetProfileInt(_T("DataStatistic"), WBSF::MEAN);
	m_TM = CTM(options.GetProfileInt(_T("DataTMType"), CTM::HOURLY));
	m_filters = CStringA(options.GetProfileString(_T("Filters")));
	std::string tmp = CStringA(options.GetProfileString(_T("ChartsPeriod")));
	m_period.FromString(tmp);
	m_bPeriodEnabled = options.GetProfileInt(_T("ChartsPeriodEnabled"), 0);
	m_chartsZoom = options.GetProfileInt(_T("ChartsZoom"), 0);
	m_variables = options.GetProfileInt(_T("Variables"));
	m_currentTab = options.GetProfileInt(_T("CurrentTab"), 0);


	std::string str = CStringA(options.GetProfileString(_T("Years")));
	m_years = WBSF::to_object<int, std::set<int>>(str," ");
}

CHourlyEditorDoc::~CHourlyEditorDoc()
{
}

BOOL CHourlyEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	//m_pStation.reset();
	m_modifiedStation.clear();
	m_outputText.clear();

	m_pDatabase.reset(new CHourlyDatabase);
	m_pStation.reset(new CWeatherStation);

	return TRUE;
}

// sérialisation de CHourlyEditorDoc

BOOL CHourlyEditorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_pDatabase.reset(new CHourlyDatabase);
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
		if (!m_period.IsInit() && !years.empty())
			m_period = CTPeriod(CTRef(*years.begin(), FIRST_MONTH, FIRST_DAY), CTRef(*years.rbegin(), LAST_MONTH, LAST_DAY));
	}
	else
	{
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}


	//UpdateAllViews(NULL, CHourlyEditorDoc::INIT, NULL);


	return (bool)msg;
}

BOOL CHourlyEditorDoc::OnSaveDocument(LPCTSTR lpszPathName)
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
	//return TRUE;
}

void CHourlyEditorDoc::OnCloseDocument()
{
	UpdateAllViews(NULL, CHourlyEditorDoc::CLOSE, NULL);

	
	//Save setting
	CAppOption options(_T("Settings"));
	options.WriteProfileInt(_T("DataTMType"), (int)m_TM.Type() );
	options.WriteProfileInt(_T("DataStatistic"), (int)m_statistic);
	options.WriteProfileString(_T("Years"), CString(WBSF::to_string(m_years, " ").c_str()));
	options.WriteProfileString(_T("Filters"), CString(m_filters.to_string().c_str()) );
	options.WriteProfileString(_T("ChartsPeriod"), CString(m_period.ToString().c_str()));
	options.WriteProfileInt(_T("ChartsPeriodEnabled"), m_bPeriodEnabled);
	options.WriteProfileInt(_T("ChartsZoom"), m_chartsZoom);
	options.WriteProfileInt(_T("Variables"), m_variables.to_ulong());
	options.WriteProfileInt(_T("CurrentTab"), m_currentTab);
	
	ASSERT(!m_bDataInEdition);
	
	ERMsg msg = m_pDatabase->Close(false);
	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	
	CDocument::OnCloseDocument();

	
}

BOOL CHourlyEditorDoc::IsModified()
{
	if (m_pDatabase == NULL)
		return FALSE;

	return !m_modifiedStation.empty();
}

BOOL CHourlyEditorDoc::SaveModified() // return TRUE if ok to continue
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
void CHourlyEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CHourlyEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Définir le contenu de recherche à partir des données du document. 
	// Les parties du contenu doivent être séparées par ";"

	// Par exemple :  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CHourlyEditorDoc::SetSearchContent(const CString& value)
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

// diagnostics pour CHourlyEditorDoc

#ifdef _DEBUG
void CHourlyEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CHourlyEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


void CHourlyEditorDoc::SetCurStationIndex(size_t i, CView* pSender)
{
	ERMsg msg;

	if (i != m_stationIndex)
	{
		m_stationIndex = i;

		if (i != UNKNOWN_POS)
		{
			CWaitCursor wait;
			assert(i < m_pDatabase->size());
			m_pStation->Reset();
			msg = m_pDatabase->Get(*m_pStation, i);
			assert(m_pStation->IsInit());
			
			if (msg)
				UpdateAllViews(pSender, STATION_INDEX_CHANGE, NULL);
			else
				SetOutputText(WBSF::GetText(msg));
		}
		else
		{
			m_pStation->Reset();
		}
	}

}


bool CHourlyEditorDoc::CancelDataEdition()
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

#include "MainFrm.h"
// commandes pour CHourlyEditorDoc
void CHourlyEditorDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CHourlyEditorDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDatabase->IsOpen() && !m_bDataInEdition);
}

void CHourlyEditorDoc::OnValidation()
{
	ERMsg msg;

	CProgressStepDlg dlg(AfxGetMainWnd());
	dlg.Create();
	dlg.DestroyWindow();

	SetOutputText("La vérification s'affichera ici un jour...");

}


void CHourlyEditorDoc::SetCurStation(CLocation& location, CView* pSender)
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

bool CHourlyEditorDoc::IsStationModified(size_t stationIndex)const
{
	return m_modifiedStation.find(stationIndex) != m_modifiedStation.end();
}

void CHourlyEditorDoc::InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc, BOOL bMakeVisible)
{
	CMainFrame* pMainFrm = (CMainFrame*)pFrame;
	pMainFrm->InitialUpdateFrame(pDoc, bMakeVisible);
}
