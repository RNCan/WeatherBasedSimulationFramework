
// DPTDoc.cpp : implémentation de la classe CMatchStationDoc
//

#include "stdafx.h"
#include <propkey.h>

#include "Basic/WeatherDatabaseCreator.h"
#include "Simulation/WeatherGradient.h"
#include "geomatic/projection.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "Simulation/WeatherGenerator.h"

#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "MainFrm.h"
#include "OutputView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace UtilWin;
using namespace WBSF;
using namespace WBSF::HOURLY_DATA;





// CMatchStationDoc
IMPLEMENT_DYNCREATE(CMatchStationDoc, CDocument)
BEGIN_MESSAGE_MAP(CMatchStationDoc, CDocument)
	ON_COMMAND(ID_VALIDATION, OnValidation)
	ON_UPDATE_COMMAND_UI(ID_VALIDATION, OnUpdateToolbar)
END_MESSAGE_MAP()


// construction ou destruction de CMatchStationDoc
CMatchStationDoc::CMatchStationDoc()
{
	CAppOption options(_T("Settings"));


	//load last session param
	m_curIndex = UNKNOWN_POS;
	m_variable = (TVarH)options.GetProfileInt(_T("Variable"));
	m_year = options.GetProfileInt(_T("Year"), -999);
	m_nbStations = options.GetProfileInt(_T("NbStations"), 4);
	m_normalsFilePath = CStringA(options.GetProfileString(_T("NormalsDatabase")));
	m_observationFilePath = CStringA(options.GetProfileString(_T("ObservationDatabase")));

	//load command line param
	const CMatchStationCmdLine& cmdInfo = theApp.m_cmdInfo;
	//const CMatchStationCmdLine* pCmdInfo = dynamic_cast <CMatchStationCmdLine*>(theApp.m_lpCmdLine);
	

	if (cmdInfo.Have(CMatchStationCmdLine::VARIABLE))
	{
		size_t var = UtilWin::ToInt64(cmdInfo.GetParam(CMatchStationCmdLine::VARIABLE));
		if (var < NB_VAR_H)
			m_variable = (TVarH)var;
	}
		

	if (cmdInfo.Have(CMatchStationCmdLine::YEAR))
		m_year = UtilWin::ToInt(cmdInfo.GetParam(CMatchStationCmdLine::YEAR));
	
	if (cmdInfo.Have(CMatchStationCmdLine::NB_STATIONS))
		m_nbStations = UtilWin::ToInt64(cmdInfo.GetParam(CMatchStationCmdLine::NB_STATIONS));

	if (cmdInfo.Have(CMatchStationCmdLine::NORMALS_FILEPATH))
		m_normalsFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::NORMALS_FILEPATH));
	
	if (cmdInfo.Have(CMatchStationCmdLine::DAILY_FILEPATH))
		m_observationFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::DAILY_FILEPATH));

	if (cmdInfo.Have(CMatchStationCmdLine::HOURLY_FILEPATH))
		m_observationFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::HOURLY_FILEPATH));
			
	m_bExecute = false;

}

CMatchStationDoc::~CMatchStationDoc()
{
}

BOOL CMatchStationDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;


	m_curIndex = UNKNOWN_POS;
	m_outputText.clear();
	m_pLocations = std::make_shared<CLocationVector>();




	return TRUE;
}
//
//void CMatchStationDoc::UpdateFrameCounts()
//{
//	CDocument::UpdateFrameCounts();
//	// sérialisation de CMatchStationDoc
//}

BOOL CMatchStationDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ERMsg msg;


	//m_bDataInEdition = false;
	m_curIndex = UNKNOWN_POS;

	m_outputText.clear();


	std::string filePath = CStringA(lpszPathName);
	std::string ext = GetFileExtension(filePath);
	//MakeLower(ext);

	if (IsEqual(ext, ".csv"))
	{
		m_locationFilePath = filePath;
	}
	else if (IsEqual(ext, ".NormalsStations"))
	{
		m_normalsFilePath = filePath;
	}
	else if (IsEqual(ext, ".DailyStations"))
	{
		m_observationFilePath = filePath;
	}
	else if (IsEqual(ext, ".HourlyStations"))
	{
		m_observationFilePath = filePath;
	}
	else if (IsEqual(ext, ".WGInput"))
	{
		m_WGFilePath = filePath;
	}

	//UpdateAllViews(NULL, CMatchStationDoc::INIT, NULL);


	return (bool)msg;
}

void CMatchStationDoc::SetLocationFilePath(LPCTSTR lpszPathName)
{
	std::string filePath = CStringA(lpszPathName);

	if (!IsEqual(filePath, m_locationFilePath))
	{
		if (FileExists(filePath) || filePath.empty())
		{
			m_locationFilePath = filePath;
			m_pLocations.reset();

			m_locationFilePath = filePath;
			if (!m_locationFilePath.empty())
			{
				ERMsg msg;

				CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				m_pLocations = std::make_shared<CLocationVector>();
				msg = m_pLocations->Load(filePath);

				dlg.DestroyWindow();

				if (!msg)
					SYShowMessage(msg, AfxGetMainWnd());
			}

			UpdateAllViews(NULL, INIT, NULL);

		}
	}
}

void CMatchStationDoc::SetNormalsFilePath(LPCTSTR lpszPathName)
{
	std::string filePath = CStringA(lpszPathName);

	if (!IsEqual(filePath, m_normalsFilePath))
	{
		if (FileExists(filePath) || filePath.empty())
		{
			if (m_pNormalsDB.get() && m_pNormalsDB->IsOpen())
				m_pNormalsDB->Close();


			m_pNormalsDB.reset();

			m_normalsFilePath = filePath;
			if (!m_normalsFilePath.empty())
			{
				ERMsg msg;

				CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				m_pNormalsDB = CreateWeatherDatabase(m_normalsFilePath);
				if (m_pNormalsDB)
					msg = m_pNormalsDB->Open(m_normalsFilePath, CNormalsDatabase::modeRead, dlg.GetCallback());
				else
					msg.ajoute("Invalid file path");


				dlg.DestroyWindow();

				if (!msg)
					SYShowMessage(msg, AfxGetMainWnd());
			}

			UpdateAllViews(NULL, NORMALS_DATABASE_CHANGE, NULL);

		}
	}
}

void CMatchStationDoc::SetObservationFilePath(LPCTSTR lpszPathName)
{
	std::string filePath = CStringA(lpszPathName);


	if (!IsEqual(filePath, m_observationFilePath))
	{
		if (FileExists(filePath) || filePath.empty())
		{
			if (m_pObservationDB.get() && m_pObservationDB->IsOpen())
				m_pObservationDB->Close();

			m_pObservationDB.reset();

			m_observationFilePath = filePath;
			if (!m_observationFilePath.empty())
			{
				ERMsg msg;
				CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				m_pObservationDB = CreateWeatherDatabase(filePath);
				if (m_pObservationDB.get())//if it's a vlid extention
					msg = m_pObservationDB->Open(filePath, CNormalsDatabase::modeRead, dlg.GetCallback());
				else
					msg.ajoute("Invalid file path");

				dlg.DestroyWindow();

				if (!msg)
				{
					m_pObservationDB.reset();
					SYShowMessage(msg, AfxGetMainWnd());
				}
			}//path not empty

			UpdateAllViews(NULL, OBSERVATION_DATABASE_CHANGE, NULL);
		}
	}

}

BOOL CMatchStationDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	//ERMsg msg;
	//if (!m_modifiedStation.empty() ||
	//	!UtilWin::FileExist(lpszPathName) )
	//{

	//	std::string filePath = CStringA(lpszPathName);
	//	
	//	if ( !m_pDatabase->IsOpen() )//create a new database
	//		msg = m_pDatabase->Open(filePath, CWeatherDatabase::modeEdit);

	//	if (msg)
	//		msg = m_pDatabase->Save();
	//		
	//	if (!msg)
	//		SYShowMessage(msg, AfxGetMainWnd());
	//}

	//return (bool)msg;
	return TRUE;
}

void CMatchStationDoc::OnCloseDocument()
{
	UpdateAllViews(NULL, CMatchStationDoc::CLOSE, NULL);


	//Save setting
	CAppOption options(_T("Settings"));
	options.WriteProfileInt(_T("NbStations"), (int)m_nbStations);
	options.WriteProfileInt(_T("Year"), m_year);
	options.WriteProfileInt(_T("Variable"), m_variable);
	options.WriteProfileString(_T("NormalsDatabase"), CString(m_normalsFilePath.c_str()));
	options.WriteProfileString(_T("ObservationDatabase"), CString(m_observationFilePath.c_str()));

	m_pLocations = std::make_shared<CLocationVector>();

	CDocument::OnCloseDocument();
}

BOOL CMatchStationDoc::IsModified()
{
	return FALSE;
}

BOOL CMatchStationDoc::SaveModified() // return TRUE if ok to continue
{
	//if (m_bDataInEdition)
	//	return FALSE;

	BOOL bSave = CDocument::SaveModified();
	//if (bSave)
	//m_modifiedStation.clear();

	return bSave;
}

#ifdef SHARED_HANDLERS

// Prise en charge des miniatures
void CMatchStationDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CMatchStationDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Définir le contenu de recherche à partir des données du document. 
	// Les parties du contenu doivent être séparées par ";"

	// Par exemple :  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CMatchStationDoc::SetSearchContent(const CString& value)
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

// diagnostics pour CMatchStationDoc

#ifdef _DEBUG
void CMatchStationDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMatchStationDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


void CMatchStationDoc::SetCurIndex(size_t i, CView* pSender)
{


	if (i != m_curIndex)
	{
		ERMsg msg;

		m_curIndex = i;

		UpdateAllViews(pSender, LOCATION_INDEX_CHANGE, NULL);
	}

}




// commandes pour CMatchStationDoc
void CMatchStationDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	//Load database
	if (m_curIndex != UNKNOWN_POS)
	{
		//update result
		if (m_pNormalsDB.get() && m_pNormalsDB->IsOpen())
		{
			//match
			CNormalsDatabasePtr pNormalsDB = std::dynamic_pointer_cast<CNormalsDatabase>(m_pNormalsDB);
			ASSERT(pNormalsDB.get());

			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == NORMALS_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
			{
				pNormalsDB->Search(m_normalsResult, GetLocation(m_curIndex), m_nbStations, m_variable, -999);
				pNormalsDB->GetStations(m_normalsResult, m_normalsStations);
			}


			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == NORMALS_DATABASE_CHANGE)
			{
				//create gradient
				m_gradient.SetNormalsDatabase(pNormalsDB);
				m_gradient.m_bForceComputeAllScale = true;
				m_gradient.m_variables = "T TR P TD";
				m_gradient.m_bXVal = false;
				m_gradient.m_target = GetLocation(m_curIndex);


				//CProgressStepDlg dlg(AfxGetMainWnd());
				//dlg.Create();

				ERMsg msg = m_gradient.CreateGradient();
			}

			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == NORMALS_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
			{
				m_normalsEstimate.Reset();
				if (GetCurIndex() != NOT_INIT)
				{
					if (m_normalsStations.size() > 0)
					{
						CNormalsStationVector stationsVector = m_normalsStations;
						stationsVector.ApplyCorrections(m_gradient);
						stationsVector.GetInverseDistanceMean(GetLocation(GetCurIndex()), m_variable, m_normalsEstimate);
					}
					//CWGInput WGInput;
					//WGInput.m_firstYear = m_year;
					//WGInput.m_lastYear = m_year;
					//WGInput.m_nbNormalsYears = 1;
					//WGInput.m_nbNormalsStations = m_nbStation;
					//WGInput.m_nbDailyStations = m_nbStation;
					//WGInput.m_nbHourlyStations = m_nbStation;

					//CWeatherGenerator WG;
					//WG.SetWGInput(WGInput);
					//WG.SetNormalDB(pNormalsDB);

					//if (m_pObservationDB && m_pObservationDB->IsOpen())
					//{
					//	if (IsEqual(m_pObservationDB->GetDatabaseExtension(), ".DailyStations"))
					//	{
					//		CDailyDatabasePtr pDailyDB = std::dynamic_pointer_cast<CDailyDatabase>(m_pObservationDB);
					//		ASSERT(pDailyDB.get());
					//		WG.SetDailyDB(pDailyDB);
					//	}
					//	else if (IsEqual(m_pObservationDB->GetDatabaseExtension(), ".HourlyStations"))
					//	{
					//		CHourlyDatabasePtr pHourlyDB = std::dynamic_pointer_cast<CHourlyDatabase>(m_pObservationDB);
					//		ASSERT(pHourlyDB.get());
					//		WG.SetHourlyDB(pHourlyDB);
					//	}
					//}

					//WG.SetTarget(GetLocation(GetCurIndex()));
					//WG.Initialize(dlg.GetCallback()); //create gradient (again)...toDo

					//msg = WG.GetNormals(m_normalsEstimate, dlg.GetCallback());

				}
			}
		}


		if (m_pObservationDB.get() && m_pObservationDB->IsOpen())
		{
			std::shared_ptr<CDHDatabaseBase> pObservationDB = std::dynamic_pointer_cast<CDHDatabaseBase>(m_pObservationDB);
			ASSERT(pObservationDB.get());

			//match
			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == OBSERVATION_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
			{
				pObservationDB->Search(m_observationResult, GetLocation(m_curIndex), m_nbStations, m_variable, m_year);
				pObservationDB->GetStations(m_observationResult, m_observationStations);
			}
		}
	}

	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->OnUpdate(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);
}

void CMatchStationDoc::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_pLocations->empty());
}

void CMatchStationDoc::OnValidation()
{
	ERMsg msg;

	CProgressStepDlg dlg(AfxGetMainWnd());
	dlg.Create();
	dlg.DestroyWindow();

	SetOutputText("La vérification s'affichera ici un jour...");

}

UINT CMatchStationDoc::Execute(void* pParam)
{
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CMatchStationDoc* pDoc = (CMatchStationDoc*)pMyParam->m_pThis;
	std::string filePath = (char*)pMyParam->m_pFilepath;
	size_t type = (size_t)pMyParam->m_pExtra;

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;
	pCallback->PushTask("Open database: " + filePath, NOT_INIT);

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
	TRY
		switch (type)
		{
		case 0:	*pMsg = pDoc->m_pNormalsDB->Open(filePath, CNormalsDatabase::modeRead, *pCallback); break;
		case 1: *pMsg = pDoc->m_pObservationDB->Open(filePath, CWeatherDatabase::modeRead, *pCallback); break;
		}
		
	CATCH_ALL(e)
		*pMsg = UtilWin::SYGetMessage(*e);
	END_CATCH_ALL

		CoUninitialize();

	pCallback->PopTask();
	if (*pMsg)
		return 0;

	return -1;
}




void CMatchStationDoc::OnInitialUpdate() // called first time after construct
{

	ERMsg msg;

	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	ENSURE(pMainFrm);
	//COutputView* pView = (COutputView*)pMainFrm->GetActiveView();
	//ENSURE(pView);
	POSITION posView = GetFirstViewPosition();
	COutputView* pView = (COutputView*)GetNextView(posView);
	ENSURE(pView);


	CProgressWnd& progressWnd = pView->GetProgressWnd();

	m_bExecute = true;
	pView->AdjustLayout();//open the progress window

	progressWnd.SetTaskbarList(pMainFrm->GetTaskbarList());

	if (!m_WGFilePath.empty() && FileExists(m_WGFilePath))
	{
		CWGInput WGInput;
		if (WGInput.Load(m_WGFilePath))
		{
			m_year = WGInput.m_firstYear;
			m_nbStations = WGInput.m_nbNormalsStations;
		}
	}

	if (m_normalsFilePath != m_lastNormalsFilePath)
	{
		m_lastNormalsFilePath = m_normalsFilePath;
		m_pNormalsDB.reset();
		if (!m_normalsFilePath.empty() && FileExists(m_normalsFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_normalsFilePath.c_str(), (void*)0);

			m_pNormalsDB = CreateWeatherDatabase(m_normalsFilePath);
			if (m_pNormalsDB)
				msg = progressWnd.Execute(Execute, &param);
			else
				msg.ajoute("Invalid Normals file path: " + m_normalsFilePath);
		}
	}

	if (m_observationFilePath != m_lastObservationFilePath)
	{
		m_lastObservationFilePath = m_observationFilePath;
		m_pObservationDB.reset();
		if (!m_observationFilePath.empty() && FileExists(m_observationFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_observationFilePath.c_str(), (void*)1);

			m_pObservationDB = CreateWeatherDatabase(m_observationFilePath);
			if (m_pObservationDB.get())//if it's a vlid extention
				msg = progressWnd.Execute(Execute, &param);
				//msg = m_pObservationDB->Open(m_observationFilePath, CNormalsDatabase::modeRead, dlg.GetCallback());
			else
				msg.ajoute("Invalid observation file path:" + m_observationFilePath);

		}//path not empty
	}

	if (m_locationFilePath != m_lastLocationFilePath)
	{
		m_lastLocationFilePath = m_locationFilePath;
		m_pLocations.reset();
		if (!m_locationFilePath.empty() && FileExists(m_locationFilePath))
		{
			m_pLocations = std::make_shared<CLocationVector>();
			msg += m_pLocations->Load(m_locationFilePath );
		}
	}

	


	//	SetNormalsFilePath(normalsFilePath);
	if(CWeatherGradient::GetShore().get()==NULL)
		msg += CWeatherGradient::SetShore(GetApplicationPath() + "Layers/shore.ann");

	m_outputText = GetOutputString(msg, progressWnd.GetCallback(), true);
	m_bExecute = false;
	pView->AdjustLayout();//open the progress window



	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());

	


	//CWeatherGradient testGradient;
	//testGradient.SetNormalsDatabase(pNormalsDB);
	//testGradient.m_variables = "T TR P TD";
	//testGradient.CreateDefaultGradient(dlg.GetCallback());
	//testGradient.Save("D:\\Travaux\\WeatherGradients\\Gradient.csv");
	//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tmin.csv", H_TAIR);
	//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tmax.csv", H_TRNG);
	//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Prcp2.csv", H_PRCP);
	//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tdew.csv", H_TDEW);


}
