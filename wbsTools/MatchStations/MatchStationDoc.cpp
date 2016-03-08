
// DPTDoc.cpp : implémentation de la classe CMatchStationDoc
//

#include "stdafx.h"


// SHARED_HANDLERS peuvent être définis dans les gestionnaires d'aperçu, de miniature
// et de recherche d'implémentation de study ATL et permettent la partage de code de document avec ce study.
#ifndef SHARED_HANDLERS
#include "MatchStationApp.h"
#endif


#include <propkey.h>

#include "Basic/WeatherDatabaseCreator.h"
#include "Simulation/WeatherGradient2.h"
#include "geomatic/projection.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "Simulation/WeatherGenerator.h"


#include "MatchStationDoc.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


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


			m_curIndex = UNKNOWN_POS;
			m_variable = (TVarH)options.GetProfileInt(_T("Variable"));
			m_year = options.GetProfileInt(_T("Year"), -999);
			m_nbStation = options.GetProfileInt(_T("NbStations"), 4);



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

			CProgressStepDlg dlg(AfxGetMainWnd());
			dlg.Create();


			m_outputText.clear();

			std::string filePath = CStringA(lpszPathName);

			m_pLocations = std::make_shared<CLocationVector>();
			msg = m_pLocations->Load(filePath);

			dlg.DestroyWindow();

			if (!msg)
				SYShowMessage(msg, AfxGetMainWnd());


			//UpdateAllViews(NULL, CMatchStationDoc::INIT, NULL);


			return (bool)msg;
		}


		void CMatchStationDoc::SetNormalsDatabase(LPCTSTR lpszPathName)
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

		void CMatchStationDoc::SetObservationDatabase(LPCTSTR lpszPathName)
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
			options.WriteProfileInt(_T("NbStations"), (int)m_nbStation);
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
						pNormalsDB->Search(m_normalsResult, GetLocation(m_curIndex), m_nbStation, m_variable, -999);
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


						CProgressStepDlg dlg(AfxGetMainWnd());
						dlg.Create();

						ERMsg msg = m_gradient.CreateGradient(dlg.GetCallback());
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
						pObservationDB->Search(m_observationResult, GetLocation(m_curIndex), m_nbStation, m_variable, m_year);
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
		

		void CMatchStationDoc::OnInitialUpdate() // called first time after construct
		{
			CAppOption options(_T("Settings"));
			CString normalsFilePath = options.GetProfileString(_T("NormalsDatabase"), _T(""));
			CString observationFilePath = options.GetProfileString(_T("ObservationDatabase"), _T(""));
			SetNormalsDatabase(normalsFilePath);
			SetObservationDatabase(observationFilePath);


			if (m_pNormalsDB.get() && m_pNormalsDB->IsOpen())
			{
				//GeoBasic::CProjection prj;
				//match

				CNormalsDatabasePtr pNormalsDB = std::dynamic_pointer_cast<CNormalsDatabase>(m_pNormalsDB);
				ASSERT(pNormalsDB.get());

				
				ERMsg msg = CWeatherGradient::SetShore(GetApplicationPath() + "Shore.ann");
				if (!msg)
					SYShowMessage(msg, ::AfxGetMainWnd());

				//CWeatherGradient::Shape2ANN("D:\\Weather\\Normals\\Shore_z3_WWS.shp", "D:\\Weather\\Normals\\Shore_z3.ann");
				//	std::string filePath = CStringA(normalsFilePath);
				//CWeatherGradient::AddShape("U:\\Geomatique\\Shapefile\\water\\water-polygons-generalized-3857\\water_polygons_z3.shp", filePath, "D:\\Weather\\Normals\\OseanStation.shp");

				//VERIFY(CWeatherGradient::SetShore("D:\\Weather\\Normals\\Shore_z3.ann"));

				/*CProgressStepDlg dlg(AfxGetMainWnd());
				dlg.Create();

				CWeatherGradient testGradient;
				testGradient.SetNormalsDatabase(pNormalsDB);
				testGradient.m_variables = "T TR P TD";
				testGradient.CreateDefaultGradient(dlg.GetCallback());
				testGradient.Save("D:\\Travaux\\WeatherGradients\\Gradient.csv");

				dlg.DestroyWindow();*/

				//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tmin.csv", H_TAIR);
				//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tmax.csv", H_TRNG);
				//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Prcp2.csv", H_PRCP);
				//testGradient.ExportInput("d:\\Travaux\\WeatherGradients\\Tdew.csv", H_TDEW);


			}
		}
}