
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
#include "Geomatic/ShoreCreator.h"

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
	m_bInit = false;
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

	return TRUE;
}




// CBioSIMDoc serialization
BOOL CBioSIMDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ENSURE(lpszPathName);

	string filePath = UtilWin::ToUTF8(lpszPathName);

	ERMsg msg = GetProject().Load(filePath);
	if (msg)
	{
		m_lastSaveProject = GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));

		//here : try to conver old loc file to new csv file
		GetFileManager().Loc().ConvertLoc2CSV();

		m_bInit = true;
		m_curSel = "Project";

		LoadProjectState(lpszPathName);
	}
	else
	{
		SYShowMessage(msg, ::AfxGetMainWnd());
	}




	return msg != 0;
}


BOOL CBioSIMDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	string filePath = UtilWin::ToUTF8(lpszPathName);
	bool newProject = !FileExists(filePath);
	ERMsg msg = GetProject().Save(filePath);
	if (msg)
	{
		m_lastSaveProject = GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));
		GetFileManager().CreateDefaultDirectories();

		//if (newProject)
		//{
		//	filePath = GetFileManager().WeatherUpdate().GetLocalPath() + "Download Current Normals Database.Update";
		//	if (!FileExists(filePath))
		//	{
		//		ofStream file;
		//		if (file.open(filePath))
		//		{
		//			file << "<?xml version=\"1.0\" encoding=\"Windows - 1252\"?>" << endl;
		//			file << "<WeatherUpdater version=\"2\">" << endl;
		//			file << "<Tasks type=\"Tools\">" << endl;
		//			file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
		//			file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
		//			file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Local\">tmp\\Canada-USA_1981-2010.zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Password\"/>" << endl;
		//			file << "<Parameters name=\"Proxy\"/>" << endl;
		//			file << "<Parameters name=\"Remote\">regniere/Data11/Weather/Normals/Canada-USA_1981-2010.zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Server\">ftp.cfl.scf.rncan.gc.ca</Parameters>" << endl;
		//			file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"UserName\"/>" << endl;
		//			file << "</Task>" << endl;
		//			file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
		//			file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Command\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
		//			file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
		//			file << "<Parameters name=\"ZipFilepath\">tmp\\Canada-USA_1981-2010.zip</Parameters>" << endl;
		//			file << "</Task>" << endl;
		//			file << "</Tasks>" << endl;
		//			file << "</WeatherUpdater>" << endl;

		//			file.close();
		//		}
		//	}

		//	CTRef today = CTRef::GetCurrentTRef();
		//	filePath = GetFileManager().WeatherUpdate().GetLocalPath() + "Download Canada " + to_string(today.GetYear() - 1) + "-" + to_string(today.GetYear()) + ".Update";
		//	if (!FileExists(filePath))
		//	{
		//		ofStream file;
		//		if (file.open(filePath))
		//		{
		//			
		//			//
		//			file << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl;
		//			file << "<WeatherUpdater version=\"2\">" << endl;
		//			file << "<Tasks type=\"Tools\">" << endl;
		//			file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
		//			file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
		//			file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Local\">tmp\\Canada_" << (today.GetYear()-1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Password\"/>" << endl;
		//			file << "<Parameters name=\"Proxy\"/>" << endl;
		//			file << "<Parameters name=\"Remote\">regniere/Data11/Weather/Daily/Canada_" << (today.GetYear() - 1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Server\">ftp.cfl.scf.rncan.gc.ca</Parameters>" << endl;
		//			file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"UserName\"/>" << endl;
		//			file << "</Task>" << endl;
		//			file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
		//			file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Command\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
		//			file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
		//			file << "<Parameters name=\"ZipFilepath\">tmp\\Canada_" << (today.GetYear() - 1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "</Task>" << endl;
		//			file << "</Tasks>" << endl;
		//			file << "</WeatherUpdater>" << endl;
		//			file.close();
		//		}
		//	}

		//	filePath = GetFileManager().WeatherUpdate().GetLocalPath() + "Download USA " + to_string(today.GetYear() - 1) + "-" + to_string(today.GetYear()) + ".Update";
		//	if (!FileExists(filePath))
		//	{
		//		ofStream file;
		//		if (file.open(filePath))
		//		{

		//			//
		//			file << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl;
		//			file << "<WeatherUpdater version=\"2\">" << endl;
		//			file << "<Tasks type=\"Tools\">" << endl;
		//			file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
		//			file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
		//			file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Local\">tmp\\USA_" << (today.GetYear() - 1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Password\"/>" << endl;
		//			file << "<Parameters name=\"Proxy\"/>" << endl;
		//			file << "<Parameters name=\"Remote\">regniere/Data11/Weather/Daily/USA_" << (today.GetYear() - 1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "<Parameters name=\"Server\">ftp.cfl.scf.rncan.gc.ca</Parameters>" << endl;
		//			file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"UserName\"/>" << endl;
		//			file << "</Task>" << endl;
		//			file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
		//			file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
		//			file << "<Parameters name=\"Command\">1</Parameters>" << endl;
		//			file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
		//			file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
		//			file << "<Parameters name=\"ZipFilepath\">tmp\\USA_" << (today.GetYear() - 1) << "-" << today.GetYear() << ".zip</Parameters>" << endl;
		//			file << "</Task>" << endl;
		//			file << "</Tasks>" << endl;
		//			file << "</WeatherUpdater>" << endl;
		//			file.close();
		//		}
		//	}
		//}

		m_bInit = true;

		SaveProjectState(lpszPathName);
	}

	return msg != 0;
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
	SetFileExtension(stateFilePath, ".states");
	m_projectState->Load(stateFilePath);
	m_curSel = "Project";

}

void CBioSIMDoc::SaveProjectState(const CString& filePath)
{
	//save state of the document
	string stateFilePath((CStringA)filePath);
	SetFileExtension(stateFilePath, ".states");
	m_projectState->Save(stateFilePath);

}
BOOL CBioSIMDoc::SaveModified() // return TRUE if ok to continue
{
	//UpdateAllViews(NULL, SEL_CHANGE);//reset all view in execution mode

	CString filePath = GetPathName();
	if (!filePath.IsEmpty())
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
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CBioSIMProject* pProject = (CBioSIMProject*)pMyParam->m_pThis;
	CFileManager* pFM = (CFileManager*)pMyParam->m_pExtra;

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
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

	if (*pMsg)
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
		if (option.GetProfileBool(_T("SaveAtRun"), false))
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
	ERMsg msg;

	if (!msg)
		SYShowMessage(msg, ::AfxGetMainWnd());

	UpdateAllViews(NULL, INIT, NULL);
}
