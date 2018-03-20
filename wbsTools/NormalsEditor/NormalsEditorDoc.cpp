
// DPTDoc.cpp : implémentation de la classe CNormalsEditorDoc
//

#include "stdafx.h"

#include "NormalsEditor.h"
#include "NormalsEditorDoc.h"
#include "Basic/Shore.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"

#include "MainFrm.h"
#include "OutputView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace WBSF;

// CNormalsEditorDoc
IMPLEMENT_DYNCREATE(CNormalsEditorDoc, CDocument)
BEGIN_MESSAGE_MAP(CNormalsEditorDoc, CDocument)
	ON_COMMAND(ID_VALIDATION, OnValidation)
	ON_UPDATE_COMMAND_UI(ID_VALIDATION, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CNormalsEditorDoc
CNormalsEditorDoc::CNormalsEditorDoc()
{
	CAppOption options(_T("Settings"));

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_bExecute = false;

}

CNormalsEditorDoc::~CNormalsEditorDoc()
{
}

BOOL CNormalsEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_pStation.reset();
	m_modifiedStation.clear();
	m_outputText.clear();

	m_pDatabase.reset(new CNormalsDatabase);
	m_pStation.reset(new CNormalsStation);

	if (CShore::GetShore().get() == NULL)
	{
		ERMsg msg;
		msg += CShore::SetShore(GetApplicationPath() + "Layers/shore.ann");

		if (!msg)
			UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return TRUE;
}

UINT CNormalsEditorDoc::OpenDatabase(void* pParam)
{
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)pMyParam->m_pThis;
	LPCTSTR lpszPathName = (LPCTSTR)pMyParam->m_pFilepath;
	std::string filePath = CStringA(lpszPathName);

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;
	pCallback->PushTask("Open database: " + filePath, NOT_INIT);

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
	TRY
		*pMsg = pDoc->m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit, *pCallback);
	CATCH_ALL(e)
		*pMsg = UtilWin::SYGetMessage(*e);
	END_CATCH_ALL

	CoUninitialize();

	pCallback->PopTask();
	if (*pMsg)
		return 0;

	return -1;
}


BOOL CNormalsEditorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;

	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	ENSURE(pMainFrm);
	POSITION posView = GetFirstViewPosition();
	COutputView* pView = (COutputView*)GetNextView(posView);
	ENSURE(pView);

	CProgressWnd& progressWnd = pView->GetProgressWnd();

	m_bExecute = true;
	pView->AdjustLayout();//open the progress window

	progressWnd.SetTaskbarList(pMainFrm->GetTaskbarList());
	CProgressStepDlgParam param(this, (void*)lpszPathName, (void*)CNormalsDatabase::modeRead);

	m_bDataInEdition = false;
	m_stationIndex = UNKNOWN_POS;
	m_pDatabase.reset(new CNormalsDatabase);
	m_pStation.reset(new CNormalsStation);
	m_modifiedStation.clear();
	m_outputText.clear();

	msg = progressWnd.Execute(OpenDatabase, &param);
	m_outputText = GetOutputString(msg, progressWnd.GetCallback(), true);


	if (msg)
	{
		CString str = GetCommandLine();
		std::string cmd_line = CStringA(str);
		std::replace(cmd_line.begin(), cmd_line.end(), '\\', '/');

		StringVector cmd;
		TokenizeWithQuote(cmd_line, ' ', cmd);
		size_t pos = cmd.Find("-ID", false);
		if (pos < cmd.size() && pos + 1 < cmd.size())
			SetCurStationIndex(m_pDatabase->GetStationIndex(cmd[pos + 1], false), NULL, false);
	}
	else
	{
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	m_bExecute = false;
	pView->AdjustLayout();//open the progress window


	return (bool)msg;
}

BOOL CNormalsEditorDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;
	if (!m_modifiedStation.empty() ||
		!UtilWin::FileExist(lpszPathName))
	{

		std::string filePath = CStringA(lpszPathName);

		if (!m_pDatabase->IsOpen())//create a new database
			msg = m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit);

		if (msg)
			msg = m_pDatabase->Save();

		if (!msg)
			UtilWin::SYShowMessage(msg, AfxGetMainWnd());
	}

	return (bool)msg;
}

void CNormalsEditorDoc::OnCloseDocument()
{
	//Save setting
	CAppOption options(_T("Settings"));

	ASSERT(!m_bDataInEdition);

	ERMsg msg = m_pDatabase->Close();
	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());

	CDocument::OnCloseDocument();
}

BOOL CNormalsEditorDoc::IsModified()
{
	if (m_pDatabase == NULL)
		return FALSE;

	return !m_modifiedStation.empty();
}

BOOL CNormalsEditorDoc::SaveModified() // return TRUE if ok to continue
{
	if (m_bDataInEdition)
		return FALSE;

	BOOL bSave = CDocument::SaveModified();
	if (bSave)
		m_modifiedStation.clear();

	return bSave;
}


#ifdef _DEBUG
void CNormalsEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNormalsEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


void CNormalsEditorDoc::SetCurStationIndex(size_t i, CView* pSender, bool bSendUpdate)
{
	ERMsg msg;

	if (i != m_stationIndex)
	{
		m_pStation->Reset();
		m_stationIndex = i;


		if (i != UNKNOWN_POS && m_pDatabase->IsOpen())
		{
			CWaitCursor wait;
			assert(i < m_pDatabase->size());
			msg = m_pDatabase->Get(*m_pStation, i);
			assert(m_pStation->IsInit());
		}

		if (bSendUpdate)
			UpdateAllViews(pSender, STATION_INDEX_CHANGE, NULL);

		if (!msg)
			SetOutputText(GetText(msg));
	}

}


bool CNormalsEditorDoc::CancelDataEdition()
{
	ERMsg msg;

	ASSERT(m_stationIndex != UNKNOWN_POS);
	ASSERT(m_stationIndex < m_pDatabase->size());
	msg = m_pDatabase->Get(*m_pStation, m_stationIndex);
	ASSERT(m_pStation->IsInit());

	if (!msg)
		SetOutputText(GetText(msg));


	return msg;
}


void CNormalsEditorDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);


	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CNormalsEditorDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDatabase->IsOpen() && !m_bDataInEdition);

}

void CNormalsEditorDoc::OnValidation()
{
	ERMsg msg;

	//CProgressStepDlg dlg;
	//dlg.Create(AfxGetMainWnd());

	//assert(false);//todo
	//msg = m_pDatabase->Open(UtilWin::ToUTF8(lpszPathName), CWeatherDatabase::modeRead, dlg.GetCallback());
	//dlg.DestroyWindow();


	SetOutputText("La vérification s'affichera ici un jour...");

}

void CNormalsEditorDoc::SetCurStation(CLocation& location, CView* pSender)
{
	ASSERT(m_pDatabase->IsOpen());
	ASSERT(m_pStation);
	ASSERT(m_stationIndex < m_pDatabase->size());

	CLocation& actualLocation = ((CLocation&)(*m_pStation));
	if (location != actualLocation)
	{
		actualLocation = location;
		m_pDatabase->Set(m_stationIndex, location);//update coordinate station info

		m_modifiedStation.insert(m_stationIndex);
		UpdateAllViews(pSender, LOCATION_CHANGE);
		SetModifiedFlag();
	}

}

bool CNormalsEditorDoc::IsStationModified(size_t stationIndex)const
{
	return m_modifiedStation.find(stationIndex) != m_modifiedStation.end();
}

//void CNormalsEditorDoc::OnInitialUpdate()
//{
//	UpdateAllViews(NULL, INIT, NULL);
//}
