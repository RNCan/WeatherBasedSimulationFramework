
// BioSIMDoc.cpp : implementation of the CBioSIMDoc class
//

#include "stdafx.h"


#include "Basic/UtilTime.h"
#include "Basic/callback.h"
#include "FileManager/FileManager.h"
#include "Simulation/WeatherGradient.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"	
#include "Geomatic/TimeZones.h"


#include "BioSIM.h"
#include "BioSIMDoc.h"
#include "MainFrm.h"
#include "OutputView.h"


using namespace std;
using namespace WBSF;
using namespace UtilWin;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//****************************************************************
// CBioSIMDoc

IMPLEMENT_DYNCREATE(CBioSIMDoc, CDocument)

BEGIN_MESSAGE_MAP(CBioSIMDoc, CDocument)
END_MESSAGE_MAP()


// CBioSIMDoc construction/destruction

CBioSIMDoc::CBioSIMDoc()
{
	m_bInit	= false;
	m_bExecute = false;
	CBioSIMProject* pProject = new CBioSIMProject;
	pProject->SetMyself(&m_projectPtr);
	m_projectPtr.reset(pProject);
	m_projectState.reset(new CProjectState);
}

CBioSIMDoc::~CBioSIMDoc()
{
}

BOOL CBioSIMDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	GetProject().Reset();
	m_lastSaveProject = GetProject();
	
	m_projectState->clear();
	m_curSel = "Project";//select project at the beginning 

	m_bInit = false;

	//UpdateAllViews(NULL, INIT);


	

/*	CStdioFile file1("D:\\project\\models\\MPB\\Hourly\\Data\\snran.txt", CFile::modeRead);
	CStdioFile file2("D:\\project\\models\\MPB\\Hourly\\Data\\snras.txt", CFile::modeRead);
	CStdioFile file3("D:\\project\\models\\MPB\\Hourly\\Data\\snra1992-2004.txt", CFile::modeWrite|CFile::modeCreate);
	file3.WriteString("Year\tJD\tTime\tNorthBl\tSouthBl\tAverBl\n");
	CString line1;
	CString line2;
	VERIFY( file1.ReadString(line1) );
	VERIFY( file2.ReadString(line2) );
	int pos1 = 0;
	int pos2 = 0;
	
	int y = 1992;
	int jd = 199;
	int h = 0;
	while( pos1!=-1 && pos2!=-1 )
	{
		CString t1 = line1.Tokenize(" ", pos1);
		CString t2 = line2.Tokenize(" ", pos2);
		if( !t1.IsEmpty() && !t2.IsEmpty() )
		{
			CString line3;
			line3.Format("%d\t%d\t%d\t%.1lf\t%.1lf\t%.1lf\n", y, jd+1, h*100,atof(t1), atof(t2), (atof(t1)+atof(t2))/2);
			file3.WriteString(line3);
		}

		h = (++h)%24;
		if( h==0)
			jd++;

		if( jd == WBSF::GetNbDay(y) )
		{
			jd=0;
			y++;
		}
	}
*/

	return TRUE;
}




// CBioSIMDoc serialization
BOOL CBioSIMDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ENSURE(lpszPathName);

	string filePath = UtilWin::ToUTF8(lpszPathName);

	ERMsg msg = GetProject().Load(filePath);
	if( msg )
	{
		m_lastSaveProject=GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));

		//here : try to conver old loc file to new csv file
		GetFileManager().Loc().ConvertLoc2CSV();

		m_bInit = true;
		m_curSel = "Project";

		LoadProjectState(lpszPathName);
		//UpdateAllViews(NULL, INIT);
	}
	else
	{
		SYShowMessage(msg, ::AfxGetMainWnd() );
	}


	

	return msg!=0;
}

	
BOOL CBioSIMDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	string filePath = UtilWin::ToUTF8(lpszPathName);
	ERMsg msg = GetProject().Save(filePath);
	if( msg )
	{
		m_lastSaveProject=GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));
		GetFileManager().CreateDefaultDirectories();
		m_bInit = true;

		SaveProjectState(lpszPathName);
	}

	return msg!=0;
}

void CBioSIMDoc::OnCloseDocument()
{
	//UpdateAllViews(NULL, CLOSE, NULL);

	CDocument::OnCloseDocument();
}

void CBioSIMDoc::LoadProjectState(const CString& filePath)
{
	//save state of the document
	string stateFilePath((CStringA)filePath);
	SetFileExtension( stateFilePath, ".states");
	m_projectState->Load(stateFilePath);
	m_curSel = "Project";

}

void CBioSIMDoc::SaveProjectState(const CString& filePath)
{
	//save state of the document
	string stateFilePath((CStringA)filePath);
	SetFileExtension( stateFilePath, ".states");
	m_projectState->Save(stateFilePath);

}
BOOL CBioSIMDoc::SaveModified() // return TRUE if ok to continue
{
	//UpdateAllViews(NULL, SEL_CHANGE);//reset all view in execution mode

	CString filePath = GetPathName();
	if( !filePath.IsEmpty() )
		SaveProjectState(filePath);

	const CBioSIMProject& project = GetProject();
	bool bModified = project != m_lastSaveProject;
	SetModifiedFlag(bModified);
	
	return CDocument::SaveModified();
}

void CBioSIMDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CBioSIMDoc diagnostics

#ifdef _DEBUG
void CBioSIMDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBioSIMDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CBioSIMDoc commands
UINT CBioSIMDoc::ExecuteTask(void* pParam)
{
	CProgressStepDlgParam* pMyParam= (CProgressStepDlgParam*)pParam;	
	CBioSIMProject* pProject = (CBioSIMProject*)pMyParam->m_pThis;
	CFileManager* pFM = (CFileManager*)pMyParam->m_pExtra;
	
	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;
	
	VERIFY( CoInitializeEx(NULL,COINIT_APARTMENTTHREADED) == S_OK);
	TRY
	{
		*pMsg = pProject->Execute(*pFM, *pCallback);
	}
	CATCH_ALL(e)
	{
		*pMsg = ::SYGetMessage(*e);
	}
	END_CATCH_ALL
	
	CoUninitialize();

	if( *pMsg )
		return 0;
	
	return -1;
}

ERMsg CBioSIMDoc::Execute(CComPtr<ITaskbarList3>& pTaskbarList)
{
	ERMsg msg;

	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	COutputView* pView = (COutputView*)pMainFrm->GetActiveView();



	SetIsExecute(true);
	pView->AdjustLayout();
	UpdateAllViews(NULL, PROJECT_CHANGE, NULL);


	GetProject().LoadDefaultCtrl();
	

	CProgressWnd& progressWnd = pView->GetProgressWnd();
	progressWnd.SetTaskbarList(pTaskbarList);

	CProgressStepDlgParam param(m_projectPtr.get(), (void*)&m_strPathName, &GetFM());

	TRY
	{
		//try to save 
		CAppOption option;
		if( option.GetProfileBool(_T("SaveAtRun"), false) )
			GetProject().Save(UtilWin::ToUTF8(m_strPathName));
		
		//progressWnd.ShowPane(TRUE, FALSE, TRUE);
		msg = progressWnd.Execute(CBioSIMDoc::ExecuteTask, &param);
		m_lastLog = SYGetOutputCString(msg, progressWnd.GetCallback());
	}
	CATCH_ALL(e)
	{
		msg = SYGetMessage(*e);
		m_lastLog = SYGetOutputCString(msg);
	}
	END_CATCH_ALL
	

	SetIsExecute(false);
	pView->AdjustLayout();
	UpdateAllViews(NULL, PROJECT_CHANGE, NULL);


	return msg;
}

void CBioSIMDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
    CDocument::SetPathName(lpszPathName, bAddToMRU);
	SetTitle(GetPathName());
}

void CBioSIMDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->UpdateAllViews(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);

}


void CBioSIMDoc::OnInitialUpdate() // called first time after construct
{
	ERMsg msg = CWeatherGradient::SetShore(GetApplicationPath() + "Layers/Shore.ann");

	msg += CTimeZones::Load(GetApplicationPath() + "zoneinfo/time_zones.shp");

	if (!msg)
		SYShowMessage(msg, ::AfxGetMainWnd());
	

	UpdateAllViews(NULL, INIT, NULL);
}