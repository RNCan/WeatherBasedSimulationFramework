
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
	m_variable = (TVarH)options.GetProfileInt(_T("Variable"), H_TAIR);
	m_searchRadius = options.GetProfileDouble(_T("SearchRadius"), -1);
	m_year = options.GetProfileInt(_T("Year"), 2015);
	m_nbStations = options.GetProfileInt(_T("NbStations"), 4);
	m_obsType = options.GetProfileInt(_T("ObservationType"), T_DAILY);
	m_bSkipVerify = options.GetProfileInt(_T("SkipVerify"), 0);
	m_hourlyFilePath = CStringA(options.GetProfileString(_T("HourlyDatabase")));
	m_dailyFilePath = CStringA(options.GetProfileString(_T("DailyDatabase")));
	m_normalsFilePath = CStringA(options.GetProfileString(_T("NormalsDatabase")));





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

	if (cmdInfo.Have(CMatchStationCmdLine::SEARCH_RADIUS))
		m_searchRadius = UtilWin::ToDouble(cmdInfo.GetParam(CMatchStationCmdLine::YEAR));

	if (cmdInfo.Have(CMatchStationCmdLine::OBS_TYPE))
		m_obsType = UtilWin::ToInt64(cmdInfo.GetParam(CMatchStationCmdLine::OBS_TYPE));

	if (cmdInfo.Have(CMatchStationCmdLine::SKIP_VERIFY))
		m_bSkipVerify = UtilWin::ToBool(cmdInfo.GetParam(CMatchStationCmdLine::OBS_TYPE));



	if (cmdInfo.Have(CMatchStationCmdLine::NORMALS_FILEPATH))
		m_normalsFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::NORMALS_FILEPATH));

	if (cmdInfo.Have(CMatchStationCmdLine::DAILY_FILEPATH))
		m_dailyFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::DAILY_FILEPATH));

	if (cmdInfo.Have(CMatchStationCmdLine::HOURLY_FILEPATH))
		m_hourlyFilePath = CStringA(cmdInfo.GetParam(CMatchStationCmdLine::HOURLY_FILEPATH));

	m_bExecute = false;


	//fill with empty 

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


	std::string filepath = CStringA(lpszPathName);
	std::string ext = GetFileExtension(filepath);
	//MakeLower(ext);

	if (IsEqual(ext, ".csv"))
	{
		m_locationFilePath = filepath;
	}
	else if (IsEqual(ext, ".NormalsStations"))
	{
		m_normalsFilePath = filepath;
	}
	else if (IsEqual(ext, ".DailyStations"))
	{
		m_dailyFilePath = filepath;
	}
	else if (IsEqual(ext, ".HourlyStations"))
	{
		m_hourlyFilePath = filepath;
	}



	return (bool)msg;
}

void CMatchStationDoc::SetLocationFilePath(const std::string& filepath)
{
	//std::string filepath = CStringA(lpszPathName);

	if (!m_bExecute && !IsEqual(filepath, m_locationFilePath))
	{
		if (FileExists(filepath) || filepath.empty())
		{
			m_locationFilePath = filepath;
			m_pLocations.reset();

			m_locationFilePath = filepath;
			if (!m_locationFilePath.empty())
			{
				ERMsg msg;


				/*CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				m_pLocations = std::make_shared<CLocationVector>();
				msg = m_pLocations->Load(filepath);

				dlg.DestroyWindow();*/

				COutputView* pView = GetOutpoutView();
				CProgressWnd& progressWnd = GetProgressWnd(pView);

				m_bExecute = true;
				pView->AdjustLayout();//open the progress window


				CProgressStepDlgParam param(this, (void*)m_locationFilePath.c_str(), (void*)T_LOCATION);

				m_pLocations = std::make_shared<CLocationVector>();
				msg = progressWnd.Execute(Execute, &param);

				m_bExecute = false;
				pView->AdjustLayout();//open the progress window



				if (!msg)
					SYShowMessage(msg, AfxGetMainWnd());
			}

			UpdateAllViews(NULL, INIT, NULL);

		}
	}
}

void CMatchStationDoc::SetNormalsFilePath(const std::string& filepath)
{
	//std::string filepath = CStringA(lpszPathName);

	if (!IsEqual(filepath, m_normalsFilePath))
	{
		if (FileExists(filepath) || filepath.empty())
		{
			if (m_pNormalsDB.get() && m_pNormalsDB->IsOpen())
				m_pNormalsDB->Close();


			m_pNormalsDB.reset();

			m_normalsFilePath = filepath;
			if (!m_normalsFilePath.empty())
			{
				ERMsg msg;

				/*CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				m_pNormalsDB = CreateWeatherDatabase(m_normalsFilePath);
				if (m_pNormalsDB)
					msg = m_pNormalsDB->Open(m_normalsFilePath, CNormalsDatabase::modeRead, dlg.GetCallback());
				else
					msg.ajoute("Invalid file path");


				dlg.DestroyWindow();
*/
				COutputView* pView = GetOutpoutView();
				CProgressWnd& progressWnd = GetProgressWnd(pView);
				m_bExecute = true;
				pView->AdjustLayout();//open the progress window

				CProgressStepDlgParam param(this, (void*)m_normalsFilePath.c_str(), (void*)T_NORMALS);

				m_pNormalsDB = CreateWeatherDatabase(m_normalsFilePath);
				if (m_pNormalsDB)
					msg = progressWnd.Execute(Execute, &param);
				else
					msg.ajoute("Invalid normals file path: " + m_normalsFilePath);

				m_bExecute = false;
				pView->AdjustLayout();//open the progress window

				if (!msg)
					SYShowMessage(msg, AfxGetMainWnd());
			}

			UpdateAllViews(NULL, NORMALS_DATABASE_CHANGE, NULL);

		}
	}
}

void CMatchStationDoc::SetDailyFilePath(const std::string& filepath)
{
	//std::string filepath = CStringA(lpszPathName);


	if (!IsEqual(filepath, m_dailyFilePath))
	{
		if (FileExists(filepath) || filepath.empty())
		{
			if (m_pDailyDB.get() && m_pDailyDB->IsOpen())
				m_pDailyDB->Close();

			m_pDailyDB.reset();

			m_dailyFilePath = filepath;
			if (!m_dailyFilePath.empty())
			{
				ERMsg msg;
				//m_obsType = T_DAILY;

				COutputView* pView = GetOutpoutView();
				CProgressWnd& progressWnd = GetProgressWnd(pView);
				m_bExecute = true;
				pView->AdjustLayout();//open the progress window

				CProgressStepDlgParam param(this, (void*)m_dailyFilePath.c_str(), (void*)T_DAILY);

				m_pDailyDB = CreateWeatherDatabase(m_dailyFilePath);
				if (m_pDailyDB.get())//if it's a valid extention
					msg = progressWnd.Execute(Execute, &param);
				else
					msg.ajoute("Invalid daily file path:" + m_dailyFilePath);

				m_bExecute = false;
				pView->AdjustLayout();//open the progress window


				if (!msg)
				{
					m_pDailyDB.reset();
					SYShowMessage(msg, AfxGetMainWnd());
				}


			}//path not empty

			UpdateAllViews(NULL, OBSERVATION_DATABASE_CHANGE, NULL);
		}
	}

}

void CMatchStationDoc::SetHourlyFilePath(const std::string& filepath)
{
	//std::string filepath = CStringA(lpszPathName);


	if (!IsEqual(filepath, m_hourlyFilePath))
	{
		if (FileExists(filepath) || filepath.empty())
		{
			if (m_pHourlyDB.get() && m_pHourlyDB->IsOpen())
				m_pHourlyDB->Close();

			m_pHourlyDB.reset();

			m_hourlyFilePath = filepath;
			if (!m_hourlyFilePath.empty())
			{
				ERMsg msg;
				//m_obsType = T_HOURLY;

				COutputView* pView = GetOutpoutView();
				CProgressWnd& progressWnd = GetProgressWnd(pView);
				CProgressStepDlgParam param(this, (void*)m_hourlyFilePath.c_str(), (void*)T_HOURLY);

				m_bExecute = true;
				pView->AdjustLayout();//open the progress window

				m_pHourlyDB = CreateWeatherDatabase(m_hourlyFilePath);
				if (m_pHourlyDB.get())//if it's a valid extention
					msg = progressWnd.Execute(Execute, &param);
				else
					msg.ajoute("Invalid hourly file path:" + m_hourlyFilePath);

				m_bExecute = false;
				pView->AdjustLayout();//open the progress window

				if (!msg)
				{
					m_pHourlyDB.reset();
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

	//	std::string filepath = CStringA(lpszPathName);
	//	
	//	if ( !m_pDatabase->IsOpen() )//create a new database
	//		msg = m_pDatabase->Open(filepath, CWeatherDatabase::modeEdit);

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
	//UpdateAllViews(NULL, CMatchStationDoc::CLOSE, NULL);


	//Save setting
	CAppOption options(_T("Settings"));
	options.WriteProfileInt(_T("NbStations"), (int)m_nbStations);
	options.WriteProfileInt(_T("Year"), m_year);
	options.WriteProfileInt(_T("Variable"), m_variable);
	options.WriteProfileDouble(_T("SearchRadius"), m_searchRadius);
	options.WriteProfileInt(_T("ObservationType"), (int)m_obsType);
	options.WriteProfileInt(_T("SkipVerify"), m_bSkipVerify);
	options.WriteProfileString(_T("NormalsDatabase"), CString(m_normalsFilePath.c_str()));
	options.WriteProfileString(_T("DailyDatabase"), CString(m_dailyFilePath.c_str()));
	options.WriteProfileString(_T("HourlyDatabase"), CString(m_hourlyFilePath.c_str()));

	m_pLocations = std::make_shared<CLocationVector>();

	CDocument::OnCloseDocument();
}

BOOL CMatchStationDoc::IsModified()
{
	return FALSE;
}

BOOL CMatchStationDoc::SaveModified() // return TRUE if ok to continue
{
	BOOL bSave = CDocument::SaveModified();

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
	if (!m_bExecute && m_curIndex != UNKNOWN_POS)
	{

		//update result
		if (m_pNormalsDB.get() && m_pNormalsDB->IsOpen())
		{
			//match
			CNormalsDatabasePtr pNormalsDB = std::dynamic_pointer_cast<CNormalsDatabase>(m_pNormalsDB);
			ASSERT(pNormalsDB.get());

			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == NORMALS_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
			{
				TVarH v = m_variable;
				if (v == H_TAIR)
					v = H_TMIN;

				pNormalsDB->Search(m_normalsResult, GetLocation(m_curIndex), m_nbStations, m_searchRadius * 1000, m_variable, -999);
				pNormalsDB->GetStations(m_normalsStations, m_normalsResult);
			}


			if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == NORMALS_DATABASE_CHANGE)
			{
				//create gradient
				m_gradient.SetNormalsDatabase(pNormalsDB);
				if (m_obsType == T_DAILY)
				{
					if (m_pDailyDB.get() && m_pDailyDB->IsOpen())
					{
						m_gradient.m_firstYear = m_year;
						m_gradient.m_lastYear = m_year;
						m_gradient.SetObservedDatabase(m_pDailyDB);
					}
				}
				else
				{
					if (m_pHourlyDB.get() && m_pHourlyDB->IsOpen())
					{
						m_gradient.m_firstYear = m_year;
						m_gradient.m_lastYear = m_year;
						m_gradient.SetObservedDatabase(m_pHourlyDB);
					}
				}


				m_gradient.m_bForceComputeAllScale = true;
				m_gradient.m_bUseShore = false;//m_tgi.m_bUseShore;
				m_gradient.m_bUseNearestShore = false;
				m_gradient.m_bUseNearestElev = true;
				m_gradient.m_variables = "TN T TX P TD Z";
				m_gradient.m_bXVal = false;
				m_gradient.m_target = GetLocation(m_curIndex);




				COutputView* pView = GetOutpoutView();
				CProgressWnd& progressWnd = GetProgressWnd(pView);

				m_bExecute = true;
				pView->AdjustLayout();//open the progress window
				CProgressStepDlgParam param(this, NULL, (void*)T_GRADIENT);


				ERMsg msg = progressWnd.Execute(Execute, &param);


				m_outputText = GetOutputString(msg, progressWnd.GetCallback(), true);
				m_bExecute = false;
				pView->AdjustLayout();//open the progress window



				if (!msg)
					UtilWin::SYShowMessage(msg, AfxGetMainWnd());


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
						stationsVector.GetInverseDistanceMean(GetLocation(GetCurIndex()), m_variable, m_normalsEstimate, true, WEATHER::SHORE_DISTANCE_FACTOR > 0);
					}

				}
			}
		}

		if (m_obsType == T_DAILY)
		{
			if (m_pDailyDB.get() && m_pDailyDB->IsOpen())
			{
				std::shared_ptr<CDHDatabaseBase> pDailyDB = std::dynamic_pointer_cast<CDHDatabaseBase>(m_pDailyDB);
				ASSERT(pDailyDB.get());

				//match
				if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == OBSERVATION_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
				{
					TVarH v = m_variable;
					if (v == H_TAIR)
						v = H_TMIN;
					pDailyDB->Search(m_dailyResult, GetLocation(m_curIndex), m_nbStations, m_searchRadius * 1000, v, m_year);
					pDailyDB->GetStations(m_dailyStations, m_dailyResult, m_year);
				}
			}
		}
		else
		{
			if (m_pHourlyDB.get() && m_pHourlyDB->IsOpen())
			{
				std::shared_ptr<CDHDatabaseBase> pHourlyDB = std::dynamic_pointer_cast<CDHDatabaseBase>(m_pHourlyDB);
				ASSERT(pHourlyDB.get());

				//match
				if (lHint == INIT || lHint == LOCATION_INDEX_CHANGE || lHint == OBSERVATION_DATABASE_CHANGE || lHint == PROPERTIES_CHANGE)
				{
					TVarH v = m_variable;
					if (v == H_TMIN || v == H_TMAX)
						v = H_TAIR;

					pHourlyDB->Search(m_hourlyResult, GetLocation(m_curIndex), m_nbStations, m_searchRadius * 1000, v, m_year);
					pHourlyDB->GetStations(m_hourlyStations, m_hourlyResult, m_year);
				}
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

	SetOutputText("La vérification s'affichera ici un jour...");

}

UINT CMatchStationDoc::Execute(void* pParam)
{
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CMatchStationDoc* pDoc = (CMatchStationDoc*)pMyParam->m_pThis;
	std::string filepath;
	if (pMyParam->m_pFilepath != NULL)
		filepath = (char*)pMyParam->m_pFilepath;

	size_t type = (size_t)pMyParam->m_pExtra;

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;
	pCallback->PushTask("Open database: " + filepath, NOT_INIT);

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
	TRY
		switch (type)
		{
		case T_HOURLY:	*pMsg = pDoc->m_pHourlyDB->Open(filepath, CWeatherDatabase::modeRead, *pCallback, pDoc->m_bSkipVerify); break;
		case T_DAILY:	*pMsg = pDoc->m_pDailyDB->Open(filepath, CWeatherDatabase::modeRead, *pCallback, pDoc->m_bSkipVerify); break;
		case T_NORMALS:	*pMsg = pDoc->m_pNormalsDB->Open(filepath, CNormalsDatabase::modeRead, *pCallback, pDoc->m_bSkipVerify); break;
		case T_LOCATION:*pMsg = pDoc->m_pLocations->Load(filepath, ";,", *pCallback); break;
		case T_GRADIENT:*pMsg = pDoc->m_gradient.CreateGradient(*pCallback); break;
		default: ASSERT(false);
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


COutputView* CMatchStationDoc::GetOutpoutView()
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	ENSURE(pMainFrm);

	POSITION posView = GetFirstViewPosition();
	COutputView* pView = (COutputView*)GetNextView(posView);
	ENSURE(pView);

	COutputView* pOutputView = (COutputView*)pView;
	return pOutputView;
}


CProgressWnd& CMatchStationDoc::GetProgressWnd(COutputView* pView)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	ENSURE(pMainFrm);

	CProgressWnd& progressWnd = pView->GetProgressWnd();
	progressWnd.SetTaskbarList(pMainFrm->GetTaskbarList());

	return progressWnd;
}


void CMatchStationDoc::OnInitialUpdate() // called first time after construct
{
	ERMsg msg;

	COutputView* pView = GetOutpoutView();
	CProgressWnd& progressWnd = GetProgressWnd(pView);

	m_bExecute = true;
	pView->AdjustLayout();//open the progress window

	if (m_normalsFilePath != m_lastNormalsFilePath)
	{
		m_lastNormalsFilePath = m_normalsFilePath;
		m_pNormalsDB.reset();
		if (!m_normalsFilePath.empty() && FileExists(m_normalsFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_normalsFilePath.c_str(), (void*)T_NORMALS);

			m_pNormalsDB = CreateWeatherDatabase(m_normalsFilePath);
			if (m_pNormalsDB)
				msg = progressWnd.Execute(Execute, &param);
			else
				msg.ajoute("Invalid normals file path: " + m_normalsFilePath);
		}
	}

	if (m_dailyFilePath != m_lastDailyFilePath)
	{
		m_lastDailyFilePath = m_dailyFilePath;
		m_pDailyDB.reset();
		if (!m_dailyFilePath.empty() && FileExists(m_dailyFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_dailyFilePath.c_str(), (void*)T_DAILY);

			m_pDailyDB = CreateWeatherDatabase(m_dailyFilePath);
			if (m_pDailyDB.get())//if it's a valid extention
				msg = progressWnd.Execute(Execute, &param);
			else
				msg.ajoute("Invalid daily file path:" + m_dailyFilePath);

		}//path not empty
	}

	if (m_hourlyFilePath != m_lastHourlyFilePath)
	{
		m_lastHourlyFilePath = m_hourlyFilePath;
		m_pHourlyDB.reset();
		if (!m_hourlyFilePath.empty() && FileExists(m_hourlyFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_hourlyFilePath.c_str(), (void*)T_HOURLY);

			m_pHourlyDB = CreateWeatherDatabase(m_hourlyFilePath);
			if (m_pHourlyDB.get())//if it's a valid extention
				msg = progressWnd.Execute(Execute, &param);
			else
				msg.ajoute("Invalid hourly file path:" + m_hourlyFilePath);

		}//path not empty
	}

	if (m_locationFilePath != m_lastLocationFilePath)
	{
		m_lastLocationFilePath = m_locationFilePath;
		m_pLocations.reset();
		if (!m_locationFilePath.empty() && FileExists(m_locationFilePath))
		{
			CProgressStepDlgParam param(this, (void*)m_locationFilePath.c_str(), (void*)T_LOCATION);

			m_pLocations = std::make_shared<CLocationVector>();
			msg = progressWnd.Execute(Execute, &param);

			//if (msg && !m_pLocations->empty())
			//	SetCurIndex(0);
			//msg += m_pLocations->Load(m_locationFilePath );
		}
	}

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
