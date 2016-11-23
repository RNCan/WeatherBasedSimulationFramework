//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include <boost/dynamic_bitset.hpp>
#include "KMeanLocal/KMlocal.h" 

#include "Basic/Callback.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "FileManager/FileManager.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/LOCGeneratorDlg.h"
#include "UI/DBEditorPropSheet.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CLOCGeneratorDlg dialog


	//const int CLOCGeneratorDlg::kMaxPoint = 99999;

	CLOCGeneratorDlg::CLOCGeneratorDlg(CWnd* pParent /*=NULL*/)
		: CDialog(CLOCGeneratorDlg::IDD, pParent),
		m_exposureCtrl(false)
	{
		CRegistry option("", "Generate LOC Dlg");

		//option.SetCurrentProfile();
		m_rect = option.GetObject<WBSF::CGeoRect>("GeoRect", WBSF::CGeoRect());
		m_exposition = option.GetValue<int>("exposition", 0);
		m_generateFrom = option.GetValue<int>("Generate from", 0);
		m_generationMethod = option.GetValue<int>("Generation method", 1);
		m_weatherGenerationMethod = option.GetValue<int>("WeatherGenerationMethod", 0);
		m_weatherStation = option.GetValue<int>("Weather station", 0);

		m_filter = option.GetValue<CWVariables>("Filter", CWVariables("T,P"));
		m_year = option.GetValue<int>("year", YEAR_NOT_INIT);
		m_nbPoint = option.GetValue<int>("nb gen", 100);
		m_nbPointLat = option.GetValue<int>("nb point lat", 10);
		m_nbPointLon = option.GetValue<int>("nb point lon", 10);
		m_nbStations = option.GetValue<int>("NbStations", 100);
		m_bElevExtrem = option.GetValue<int>("bExtrem", 0);
		m_factor = option.GetValue<float>("factor", 0);
		m_genType = option.GetValue<int>("gen expo type", 1);

		m_DEMName = CString(option.GetProfileString("DEM Name", "").c_str());
		m_normalDBName = CString(option.GetProfileString("NormalDBName", "").c_str());
		m_dailyDBName = CString(option.GetProfileString("DailyDBName", "").c_str());
		m_hourlyDBName = CString(option.GetProfileString("HourlyDBName", "").c_str());
		m_useBoundingBox = option.GetProfileBool("UseBoundingBox", false);
	}


	void CLOCGeneratorDlg::DoDataExchange(CDataExchange* pDX)
	{

		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_GENLOC_FILTER, m_filterCtrl);
		DDX_WVariables(pDX, IDC_GENLOC_FILTER, m_filter);
		DDX_Control(pDX, IDC_GENLOC_EXPOSURE, m_exposureCtrl);
		DDX_Control(pDX, IDC_GENLOC_BOUNDINGBOX, m_rectCtrl);
		DDX_GeoRect(pDX, IDC_GENLOC_BOUNDINGBOX, m_rect);
		CAutoEnableStatic::DDX_Check(pDX, IDC_GENLOC_BOUNDINGBOX, m_useBoundingBox);
		DDX_Control(pDX, IDC_GENLOC_NBPOINT, m_nbPointCtrl);
		DDX_Control(pDX, IDC_GENLOC_NBPOINT_LAT, m_nbPointLatCtrl);
		DDX_Control(pDX, IDC_GENLOC_NBPOINT_LON, m_nbPointLonCtrl);
		DDX_Control(pDX, IDC_GENLOC_NBSTATION, m_nbStationCtrl);
		DDX_Control(pDX, IDC_GENLOC_YEAR, m_yearCtrl);
		DDX_Radio(pDX, IDC_GENLOC_NO_EXPOSITION, m_exposition);
		CString str = m_year > YEAR_NOT_INIT ? UtilWin::ToCString(m_year) : _T("");
		DDX_Text(pDX, IDC_GENLOC_YEAR, str);
		m_year = str.IsEmpty() ? YEAR_NOT_INIT : UtilWin::ToInt(str);
		DDX_Text(pDX, IDC_GENLOC_NBPOINT, m_nbPoint);
		DDX_Text(pDX, IDC_GENLOC_NBPOINT_LAT, m_nbPointLat);
		DDX_Text(pDX, IDC_GENLOC_NBPOINT_LON, m_nbPointLon);
		DDX_Text(pDX, IDC_GENLOC_NBSTATION, m_nbStations);
		DDX_Radio(pDX, IDC_GENLOC_UNIFORM, m_genType);
		DDX_CBIndex(pDX, IDC_GENLOC_FROM, m_generateFrom);
		DDX_CBIndex(pDX, IDC_GENLOC_METHOD, m_generationMethod);
		DDX_CBIndex(pDX, IDC_GENLOC_METHOD_W, m_weatherGenerationMethod);
		DDX_CBIndex(pDX, IDC_GENLOC_STATION_TYPE, m_weatherStation);
		DDX_CBStringExact(pDX, IDC_GENLOC_DEM_NAME, m_DEMName);
		DDX_CBStringExact(pDX, IDC_GENLOC_NORMAL_NAME, m_normalDBName);
		DDX_CBStringExact(pDX, IDC_GENLOC_DAILY_NAME, m_dailyDBName);
		DDX_CBStringExact(pDX, IDC_GENLOC_HOURLY_NAME, m_hourlyDBName);
		DDX_Check(pDX, IDC_GENLOC_EXTREM, m_bElevExtrem);
		DDX_Text(pDX, IDC_GENLOC_FACTOR, m_factor);
		
		/*if (pDX->m_bSaveAndValidate)
			m_method = GetGenerationMethod();
		else
			SetGenerationMethod(m_method);*/
	}


	BEGIN_MESSAGE_MAP(CLOCGeneratorDlg, CDialog)
		ON_BN_CLICKED(IDC_GENLOC_SELECTDEM, OnSelectDEM)
		ON_CBN_SELCHANGE(IDC_GENLOC_METHOD, UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_GENLOC_METHOD_W, UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_GENLOC_STATION_TYPE, UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_GENLOC_FROM, OnGenerateFromChange)
		ON_BN_CLICKED(IDC_GENLOC_NO_EXPOSITION, OnExpoTypeChange)
		ON_BN_CLICKED(IDC_GENLOC_EXPOSITION, OnExpoTypeChange)
		ON_BN_CLICKED(IDC_GENLOC_EXPOSITION_FROM_DEM, OnExpoTypeChange)
		ON_WM_DESTROY()
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CLOCGeneratorDlg msg handlers

	BOOL CLOCGeneratorDlg::OnInitDialog()
	{
		FillDEMList();
		FillNormalDBList();
		FillDailyDBList();
		FillHourlyDBList();

		CDialog::OnInitDialog();

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CLOCGeneratorDlg::OnOK()
	{
		if (!UpdateData()) return;

		CWaitCursor waitCursor;

		TGenerationFrom generateFrom = GetGenerateFromType();
		TWeatherStation stationType = GetWeatherStationType();
		TGenerationMethod method = GetGenerationMethod();

		m_locArray.clear();

		ERMsg msg;

		switch (generateFrom)
		{
		case FROM_DEM: msg = GenerateFromDEM(); break;
		case FROM_WEATHER: msg = GenerateFromWeatherStation(); break;
		default: ASSERT(false);
		};

		if (!msg)
		{
			UtilWin::SYShowMessage(msg, this);
			return;
		}

		//CDialog::OnOK();
		EndDialog(IDOK);
	}

	UINT CLOCGeneratorDlg::ExecuteGenerateFromDEM(void* pParam)
	{
		CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
		CLOCGeneratorDlg* pDlg = (CLOCGeneratorDlg*)pMyParam->m_pThis;
		string* pFilePath = (string*)pMyParam->m_pFilepath;
		//CFileManager* pFM = (CFileManager*)pMyParam->m_pExtra;
		TGenerationMethod* pMethod = (TGenerationMethod*)pMyParam->m_pExtra;

		ERMsg* pMsg = pMyParam->m_pMsg;
		CCallback* pCallback = pMyParam->m_pCallback;

		VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
		TRY
			//*pMsg = pProject->Execute(*pFM, *pCallback);
			CGDALDatasetEx grid;
			*pMsg = grid.OpenInputImage(*pFilePath);

			if (*pMsg)
			{
				CGridPointVector pointArray;
				
				bool bExp = (pDlg->m_exposition == EXPO_FROM_DEM); //generate from DEM

				if (*pMethod == REGULAR)
					*pMsg = grid.GetRegularCoord(pDlg->m_nbPointLon, pDlg->m_nbPointLat, bExp, pDlg->m_useBoundingBox ? pDlg->m_rect : CGeoRect(), pointArray, *pCallback);
				else
					*pMsg = grid.GetRandomCoord(pDlg->m_nbPoint, bExp, pDlg->m_bElevExtrem, pDlg->m_factor / 100, pDlg->m_useBoundingBox ? pDlg->m_rect : CGeoRect(), pointArray, *pCallback);


				//on tranfers les pts dans les LOC
				if (*pMsg)
				{
					pCallback->PushTask("Final Computation", pointArray.size());

					CProjectionPtr pPrj = grid.GetPrj();
					CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));
					if (!IsGeographic(pointArray.GetPrjID()))
						pointArray.Reproject(PT);

					for (size_t i = 0; i < pointArray.size() && *pMsg; i++)
					{
						CLocation station;
						string tmp = FormatA("Pt_%06d", i + 1);

						station.m_name = tmp;
						station.m_ID = ToString(i + 1);
						station.m_lat = pointArray[i].m_lat;
						station.m_lon = pointArray[i].m_lon;
						station.m_alt = pointArray[i].m_alt;

						if (pDlg->m_exposition == EXPO_GENERATE)
						{
							int expoFactor = pDlg->m_genType == EXPO_UNIFORM ? 1 : 30;

							ASSERT(expoFactor >= 1 && expoFactor <= 60);
							WBSF::CStatistic stat;

							for (int j = 0; j < expoFactor; j++)
								stat += WBSF::Rand(0, 60);

							pointArray[i].m_slope = tan(WBSF::Deg2Rad(stat[WBSF::MEAN])) * 100;
							pointArray[i].m_aspect = WBSF::Rand(0, 359);
						}

						if (pDlg->m_exposition != NO_EXPO)
						{
							station.SetDefaultSSI(CLocation::SLOPE, ToString(pointArray[i].m_slope));
							station.SetDefaultSSI(CLocation::ASPECT, ToString(pointArray[i].m_aspect));
						}

						pDlg->m_locArray.push_back(station);
						*pMsg += pCallback->StepIt();

					}//for all

					pCallback->PopTask();
				}//if msg
			}//if msg

			grid.Close();
		CATCH_ALL(e)
			*pMsg = UtilWin::SYGetMessage(*e);
		END_CATCH_ALL

			CoUninitialize();

		if (*pMsg)
			return 0;

		return -1;
	}
	

	
	
	ERMsg CLOCGeneratorDlg::GenerateFromDEM()
	{
		ERMsg msg;

		msg = VerifyParameter();

		if (msg)
		{
			string DEMFileName = GetDEMNameCtrl().GetString();

			if (!DEMFileName.empty())
			{

				//WBSF::CCallback callBack;
				CProgressStepDlg progressDlg;
				progressDlg.Create(this);

				string filePath = WBSF::GetFM().MapInput().GetFilePath(DEMFileName);

				TGenerationMethod method = GetGenerationMethod();


				CProgressStepDlgParam param(this, &filePath, &method);
				msg = progressDlg.Execute(ExecuteGenerateFromDEM, &param);

			//	CGDALDatasetEx grid;
			//	msg = grid.OpenInputImage(filePath);

			//	if (msg)
			//	{
			//		CGridPointVector pointArray;

			//		TGenerationMethod method = GetGenerationMethod();
			//		bool bExp = (m_exposition == EXPO_FROM_DEM); //generate from DEM

			//		if (method == REGULAR)
			//			msg = grid.GetRegularCoord(m_nbPointLon, m_nbPointLat, bExp, m_useBoundingBox ? m_rect : CGeoRect(), pointArray, progressDlg.GetCallback());
			//		else
			//			msg = grid.GetRandomCoord(m_nbPoint, bExp, m_bElevExtrem, m_factor / 100, m_useBoundingBox ? m_rect : CGeoRect(), pointArray, progressDlg.GetCallback());


			//		//on tranfers les pts dans les LOC
			//		if (msg)
			//		{
			//			CProjectionPtr pPrj = grid.GetPrj();
			//			CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));
			//			if (!IsGeographic(pointArray.GetPrjID()))
			//				pointArray.Reproject(PT);

			//			for (int i = 0; i < pointArray.size(); i++)
			//			{
			//				CLocation station;
			//				string tmp = FormatA("Pt_%06d", i + 1);

			//				station.m_name = tmp;
			//				station.m_ID = ToString(i + 1);
			//				station.m_lat = pointArray[i].m_lat;
			//				station.m_lon = pointArray[i].m_lon;
			//				station.m_alt = pointArray[i].m_alt;

			//				if (m_exposition == EXPO_GENERATE)
			//				{
			//					int expoFactor = m_genType == EXPO_UNIFORM ? 1 : 30;

			//					ASSERT(expoFactor >= 1 && expoFactor <= 60);
			//					WBSF::CStatistic stat;

			//					for (int j = 0; j < expoFactor; j++)
			//						stat += WBSF::Rand(0, 60);

			//					pointArray[i].m_slope = tan(WBSF::Deg2Rad(stat[WBSF::MEAN])) * 100;
			//					pointArray[i].m_aspect = WBSF::Rand(0, 359);
			//				}

			//				if (m_exposition != NO_EXPO)
			//				{
			//					station.SetDefaultSSI(CLocation::SLOPE, ToString(pointArray[i].m_slope));
			//					station.SetDefaultSSI(CLocation::ASPECT, ToString(pointArray[i].m_aspect));
			//				}

			//				m_locArray.push_back(station);
			//			}
			//		}
			//	}

			//	grid.Close();
				progressDlg.DestroyWindow();

			}
			else
			{
				msg.ajoute(GetString(IDS_SIM_GENLOC_NODEM));
			}
		}

		return msg;
	}




	//1- pour tous les stations disponibles, faire une liste des points qui sont le plus près l'un de l'autre
	//2- De cette liste: éliminer la stations avec le moins de données de chaque paire dont la distance est plus petite que la médiane tant que le nombre de station est suppéreiur au nombre de station désiré.
	//3- répéter l'opération jusqu'à l'obtention du nombre de station désiré. Vérifier qu'on n'entre pas dans une boucle sans fin.
	//
	ERMsg GenerateWellDistributedStation(int nbStations, vector<size_t> priority, const CLocationVector& pointIn, CLocationVector& pointOut, WBSF::CCallback& callback)
	{
		assert(priority.size() == pointIn.size());

		ERMsg msg;


		//callback.AddMessage(GetString(IDS_CREATE_DATABASE));
		//callback.AddMessage(GetAbsoluteFilePath(m_outputFilePath), 1);

		//	string outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
		//SetFileExtension(outputFilePath, ".csv");

		//CLocationVector locInput;


		//	msg += locInput.Load(GetAbsoluteFilePath(m_inputFilePath));

		//if (!msg)
		//return msg;

		double a = log(pointIn.size()) / log(4);
		double b = log(nbStations) / log(4);
		size_t c = ceil(a - b);

		callback.PushTask("Generate Well Distributed Station", c);


		//vector<size_t> priority;
		//pWeatherDB->GetPriority(priority, filter, year);

		//put location in the ANN class
		//Create histogram of frequencies
		CSearchResultVector resultDistance;
		resultDistance.resize(pointIn.size());

		boost::dynamic_bitset<size_t> status(pointIn.size());
		status.set();

		size_t step = 0;
		while (status.count() > nbStations&&msg)
		{
			step++;
			callback.PushTask("Eliminate points: step" + ToString(step), pointIn.size() * 3);
			//callback.SetNbStep(pointIn.size() * 3);

			callback.AddMessage("Number of station left: " + ToString(status.count()));

			CLocationVector locations;
			vector<__int64> positions;
			for (size_t j = 0; j < pointIn.size() && msg; j++)
			{
				if (status[j])
				{
					locations.push_back(pointIn[j]);
					positions.push_back(j);
				}
				msg += callback.StepIt();
			}

			CApproximateNearestNeighbor ann;
			ann.set(locations, true, positions);

			CStatisticEx stats;
			CSearchResultVector result;
			result.resize(pointIn.size());
			for (size_t j = 0; j < pointIn.size() && msg; j++)
			{
				if (status[j])
				{
					CSearchResultVector tmp;
					if (ann.search(pointIn[j], 2ull, tmp))
					{
						ASSERT(tmp.size() == 2);
						ASSERT(tmp[0].m_index == j);
						result[j] = tmp[1];
						stats += result[j].m_distance;
					}
				}

				msg += callback.StepIt();
			}

			double median = stats[WBSF::MEDIAN];

			//find pair
			for (size_t j = 0; j < pointIn.size() && status.count()>nbStations&&msg; j++)
			{
				if (status[j])
				{
					size_t jj = result[j].m_index;
					if (result[jj].m_index == j && status[jj])
					{
						ASSERT(result[j].m_distance == result[jj].m_distance);
						if (result[j].m_distance < median)
						{
							size_t priority1 = priority[j];
							size_t priority2 = priority[jj];
							if (priority1 < priority2)
								status.reset(j);
							else
								status.reset(jj);
						}
					}
				}

				msg += callback.StepIt();
			}

			callback.PopTask();
		}


		if (msg)
		{
			ASSERT(status.count() == nbStations);

			pointOut.reserve(nbStations);
			for (size_t i = 0; i < status.size(); i++)
			{
				if (status[i])
					pointOut.push_back(pointIn[i]);
			}
		}

		callback.PopTask();

		return msg;
	}
	//
	//void GetClusterNode(CWeatherDatabasePtr& pWeatherDB, int nbCluster, CWVariables filter, int year, vector<size_t>& priority, CLocationVector& pointIn, CLocationVector& pointOut)
	//{
	//
	//	KMterm	term(100, 0, 0, 0,		// run for 100 stages
	//		0.10,			// min consec RDL
	//		0.10,			// min accum RDL
	//		3,				// max run stages
	//		0.50,			// init. prob. of acceptance
	//		100,			// temp. run length
	//		0.95);			// temp. reduction factor
	//
	//	//	term.setAbsMaxTotStage(stages);		// set number of stages
	//
	//	int nbDim = 4;
	//	//CStatisticVector stats(nbDim);
	//	KMdata dataPts(nbDim, (int)pointIn.size());		// allocate data storage
	//	for (int i = 0; i<pointIn.size(); i++)
	//	{
	//		
	//		double xx = WBSF::Deg2Rad(pointIn[i].m_x + 180);
	//		double yy = WBSF::Deg2Rad(90 - pointIn[i].m_y);
	//
	//		dataPts[i][0] =  6371 * 1000 * cos(xx)*sin(yy); 
	//		dataPts[i][1] =  6371 * 1000 * sin(xx)*sin(yy); 
	//		dataPts[i][2] =  6371 * 1000 * cos(yy); 
	//		dataPts[i][3] =  pointIn[i].m_z*100; 
	//	}
	//
	//	//normalization of data???
	//	//for (int i = 0; i<dataPts.getNPts(); i++)
	//	//{
	//	//	for (int j = 0; j<dataPts.getDim(); j++)
	//	//	{
	//	//		dataPts[i][j] = (dataPts[i][j] - stats[j][MEAN]) / stats[j][WBSF::STD_DEV];
	//	//	}
	//	//}
	//
	//	dataPts.buildKcTree();			// build filtering structure
	//
	//	KMfilterCenters ctrs(nbCluster, dataPts);		// allocate centers
	//	
	//
	//	//CTimer timer(true);
	//	// run each of the algorithms
	//	//cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
	//	//KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
	//	//ctrs = kmLloyds.execute();			// execute
	//	//timer.Stop();   
	//	//printSummary("Lloyds",kmLloyds, dataPts, ctrs);	// print summary
	//	//cout << timer.Elapsed() << " s" << endl;
	//
	////		timer.Start(true);
	//	//cout << "\nExecuting Clustering Algorithm: Swap\n";
	//	KMlocalSwap kmSwap(ctrs, term);		// Swap heuristic
	//	ctrs = kmSwap.execute();
	//	//timer.Stop();
	//	//printSummary("Swap", kmSwap, dataPts, ctrs);
	//	//cout << timer.Elapsed() << " s" << endl;
	//	/*
	//	timer.Start(true);
	//	cout << "\nExecuting Clustering Algorithm: EZ-Hybrid\n";
	//	KMlocalEZ_Hybrid kmEZ_Hybrid(ctrs, term);	// EZ-Hybrid heuristic
	//	ctrs = kmEZ_Hybrid.execute();
	//	timer.Stop();
	//	printSummary("EZ-Hybrid", kmEZ_Hybrid, dataPts, ctrs);
	//	cout << timer.Elapsed() << " s" << endl;
	//
	//	timer.Start(true);
	//	cout << "\nExecuting Clustering Algorithm: Hybrid\n";
	//	KMlocalHybrid kmHybrid(ctrs, term);		// Hybrid heuristic
	//	ctrs = kmHybrid.execute();
	//	timer.Stop();
	//	printSummary("Hybrid", kmHybrid, dataPts, ctrs);
	//	cout << timer.Elapsed() << " s" << endl;
	//	*/
	//
	//
	//
	//	pointOut.resize(ctrs.getK());
	//	//pointOut.m_bGeographic = bGeographic;
	//	//KMcenterArray clusterPts = ctrs.getCtrPts();
	//	
	//	
	//	for (int i = 0; i < ctrs.getK(); i++)
	//	{
	//		CLocation loc;
	//		loc.m_x = WBSF::Rad2Deg(atan2(ctrs[i][1], ctrs[i][0])) - 180;
	//		loc.m_y = 90 - WBSF::Rad2Deg(acos(ctrs[i][2] / (6371 * 1000)));
	//		loc.m_z = ctrs[i][3] / 100;
	//	
	//		//Get nearest station of the center
	//		CSearchResultVector result;
	//		pWeatherDB->Search(result, loc, 1ull, filter, year, true);
	//		pointOut[i] = pWeatherDB->GetLocation(result[0].m_index);
	//	}
	//
	//	KMpointArray test = ctrs.getDataPts();
	//	for (int i = 0; i < ctrs.getNPts(); i++)
	//	{
	//		double t = test[i][0];
	//	}
	//	
	//	//compute of all priority
	//	//for (int i = 0; i < pointIn.size(); i++)
	//	//{
	//	//	dataPts[0].
	//	//}
	//	
	//}

	void GetClusterNode(CWeatherDatabasePtr& pWeatherDB, int nbCluster, CWVariables filter, int year, CLocationVector& pointIn, CLocationVector& pointOut)
	{
		KMterm	term(100, 0, 0, 0,		// run for 100 stages
			0.10,			// min consec RDL
			0.10,			// min accum RDL
			3,				// max run stages
			0.50,			// init. prob. of acceptance
			100,			// temp. run length
			0.95);			// temp. reduction factor

		//	term.setAbsMaxTotStage(stages);		// set number of stages

		int nbDim = 5;
		//CStatisticVector stats(nbDim);
		KMdata dataPts(nbDim, (int)pointIn.size());		// allocate data storage
		for (int i = 0; i < pointIn.size(); i++)
		{
			double xx = WBSF::Deg2Rad(pointIn[i].m_x + 180);
			double yy = WBSF::Deg2Rad(90 - pointIn[i].m_y);

			dataPts[i][0] = 6371 * 1000 * cos(xx)*sin(yy);
			dataPts[i][1] = 6371 * 1000 * sin(xx)*sin(yy);
			dataPts[i][2] = 6371 * 1000 * cos(yy);
			dataPts[i][3] = pointIn[i].m_z * 100;
		}

		dataPts.buildKcTree();			// build filtering structure

		KMfilterCenters ctrs(nbCluster, dataPts);		// allocate centers


		//CTimer timer(true);
		// run each of the algorithms
		//cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
		//KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
		//ctrs = kmLloyds.execute();			// execute
		//timer.Stop();   
		//printSummary("Lloyds",kmLloyds, dataPts, ctrs);	// print summary
		//cout << timer.Elapsed() << " s" << endl;

		//		timer.Start(true);
		//cout << "\nExecuting Clustering Algorithm: Swap\n";
		KMlocalSwap kmSwap(ctrs, term);		// Swap heuristic
		ctrs = kmSwap.execute();
		//timer.Stop();
		//printSummary("Swap", kmSwap, dataPts, ctrs);
		//cout << timer.Elapsed() << " s" << endl;
		/*
		timer.Start(true);
		cout << "\nExecuting Clustering Algorithm: EZ-Hybrid\n";
		KMlocalEZ_Hybrid kmEZ_Hybrid(ctrs, term);	// EZ-Hybrid heuristic
		ctrs = kmEZ_Hybrid.execute();
		timer.Stop();
		printSummary("EZ-Hybrid", kmEZ_Hybrid, dataPts, ctrs);
		cout << timer.Elapsed() << " s" << endl;

		timer.Start(true);
		cout << "\nExecuting Clustering Algorithm: Hybrid\n";
		KMlocalHybrid kmHybrid(ctrs, term);		// Hybrid heuristic
		ctrs = kmHybrid.execute();
		timer.Stop();
		printSummary("Hybrid", kmHybrid, dataPts, ctrs);
		cout << timer.Elapsed() << " s" << endl;
		*/



		pointOut.resize(ctrs.getK());
		//pointOut.m_bGeographic = bGeographic;
		//KMcenterArray clusterPts = ctrs.getCtrPts();


		for (int i = 0; i < ctrs.getK(); i++)
		{
			//double test[3] = { clusterPts[i][0], clusterPts[i][1], clusterPts[i][2] };
			CLocation loc;
			loc.m_x = WBSF::Rad2Deg(atan2(ctrs[i][1], ctrs[i][0])) - 180;
			loc.m_y = 90 - WBSF::Rad2Deg(acos(ctrs[i][2] / (6371 * 1000)));
			loc.m_z = ctrs[i][3] / 100;

			//loc.m_x = ctrs[i][0];
			//loc.m_y = ctrs[i][1];
			//loc.m_z = ctrs[i][2];

			//Get nearest station of the center
			CSearchResultVector result;
			pWeatherDB->Search(result, loc, 1, -1, filter, year, true);
			pointOut[i] = pWeatherDB->GetLocation(result[0].m_index);

			//ptTmp.SetXY(dataPts[i][0] * stats[0][WBSF::STD_DEV] + stats[0][MEAN], dataPts[i][1] * stats[1][WBSF::STD_DEV] + stats[1][MEAN], dataPts[i][2] * stats[2][WBSF::STD_DEV] + stats[2][MEAN]);
			//ptTmp.m_z = dataPts[i][3] * stats[3][WBSF::STD_DEV] + stats[3][MEAN];
		}

	}



	UINT CLOCGeneratorDlg::ExecuteGenerateFromWeather(void* pParam)
	{
		CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
		CLOCGeneratorDlg* pDlg = (CLOCGeneratorDlg*)pMyParam->m_pThis;
		string* pFilepaht = (string*)pMyParam->m_pFilepath;
		TWeatherStation* pMethod = (TWeatherStation *)pMyParam->m_pExtra;

		ERMsg* pMsg = pMyParam->m_pMsg;
		CCallback* pCallback = pMyParam->m_pCallback;

		VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
		TRY

		CWeatherDatabasePtr pWeatherDB = CreateWeatherDatabase(*pFilepaht);
		if (pWeatherDB)
		{
			*pMsg = pWeatherDB->Open(*pFilepaht, CWeatherDatabase::modeRead, *pCallback);
			if (*pMsg)
			{
				CSearchResultVector searchResult;
				*pMsg = pWeatherDB->GetStationList(searchResult, pDlg->m_filter, pDlg->m_year, true, pDlg->m_useBoundingBox ? pDlg->m_rect : CGeoRect());

				if (*pMsg)
				{
					if (pDlg->m_weatherGenerationMethod == ALL_STATIONS)
					{
						pDlg->m_locArray = pWeatherDB->GetLocations(searchResult);
					}
					else if (pDlg->m_weatherGenerationMethod == MOST_COMPLETE_STATIONS)
					{
						if (searchResult.size() > pDlg->m_nbStations)
						{
							vector<size_t> order;
							*pMsg = pWeatherDB->GetStationOrder(order, pDlg->m_filter);

							for (vector<size_t>::iterator it = order.begin(); it != order.end();)
							{
								if (std::find(searchResult.begin(), searchResult.end(), *it) == searchResult.end())
									it = order.erase(it);
								else
									it++;
							}


							//eliminate unselected stations
							order.resize(pDlg->m_nbStations);
							pDlg->m_locArray.resize(pDlg->m_nbStations);
							for (size_t i = 0; i != order.size(); i++)
								pDlg->m_locArray[i] = pWeatherDB->GetLocation(order[i]);
						}

					}
					else if (pDlg->m_weatherGenerationMethod == WELL_DISTRIBUTED_STATIONS)
					{
						if (searchResult.size() > pDlg->m_nbStations)
						{
							CLocationVector locIn = pWeatherDB->GetLocations(searchResult);
							GetClusterNode(pWeatherDB, pDlg->m_nbStations, pDlg->m_filter, pDlg->m_year, locIn, pDlg->m_locArray);
						}
					}
					else if (pDlg->m_weatherGenerationMethod == COMPLETE_AND_DISTRIBUTED_STATIONS)
					{
						if (searchResult.size() > pDlg->m_nbStations)
						{
							vector<size_t> priority;
							for (CSearchResultVector::iterator it = searchResult.begin(); it != searchResult.end(); it++)
							{
								CWVariablesCounter counter = pWeatherDB->GetWVariablesCounter(it->m_index, pDlg->m_year);
								if (pDlg->m_filter.any())
								{
									for (size_t v = 0; v < NB_VAR_H; v++)
										if (!pDlg->m_filter[v])
											counter[v] = CCountPeriod();
								}

								priority.push_back(counter.GetSum());
							}

							CLocationVector locIn = pWeatherDB->GetLocations(searchResult);
							//éliminate unselected stations
							GenerateWellDistributedStation(pDlg->m_nbStations, priority, locIn, pDlg->m_locArray, *pCallback);
						}
					}

					pWeatherDB->Close();
				}
			}
		}
		CATCH_ALL(e)
		{
			*pMsg = UtilWin::SYGetMessage(*e);
		}
		END_CATCH_ALL

			CoUninitialize();

		if (*pMsg)
			return 0;

		return -1;
	}


	ERMsg CLOCGeneratorDlg::GenerateFromWeatherStation()
	{
		ERMsg msg;

		CProgressStepDlg progressDlg;
		progressDlg.Create(this);

		TWeatherStation method = GetWeatherStationType();
		

		string DBName;
		switch (method)
		{
		case NORMAL:	DBName = GetNormalDBNameCtrl().GetString(); break;
		case DAILY:		DBName = GetDailyDBNameCtrl().GetString(); break;
		case HOURLY:	DBName = GetHourlyDBNameCtrl().GetString(); break;
		}

		string DBFilePath;
		switch (method)
		{
		case NORMAL:	DBFilePath = WBSF::GetFM().Normals().GetFilePath(DBName); break;
		case DAILY:		DBFilePath = WBSF::GetFM().Daily().GetFilePath(DBName); break;
		case HOURLY:	DBFilePath = WBSF::GetFM().Hourly().GetFilePath(DBName); break;
		}

		CProgressStepDlgParam param(this, &DBFilePath, &method);
		msg = progressDlg.Execute(ExecuteGenerateFromWeather, &param);

		progressDlg.DestroyWindow();

		//CWeatherDatabasePtr pWeatherDB = CreateWeatherDatabase(DBFilePath);

		//msg = pWeatherDB->Open(DBFilePath, CWeatherDatabase::modeRead, progressDlg.GetCallback());
		//if (msg)
		//{
		//	CSearchResultVector searchResult;
		//	msg = pWeatherDB->GetStationList(searchResult, m_filter, m_year, true, m_useBoundingBox ? m_rect : CGeoRect());

		//	if (msg)
		//	{
		//		if (m_weatherGenerationMethod == ALL_STATIONS)
		//		{
		//			m_locArray = pWeatherDB->GetLocations(searchResult);
		//		}
		//		else if (m_weatherGenerationMethod == MOST_COMPLETE_STATIONS)
		//		{
		//			if (searchResult.size() > m_nbStations)
		//			{
		//				vector<size_t> order;
		//				msg = pWeatherDB->GetStationOrder(order, m_filter);

		//				for (vector<size_t>::iterator it = order.begin(); it != order.end();)
		//				{
		//					if (std::find(searchResult.begin(), searchResult.end(), *it) == searchResult.end())
		//						it = order.erase(it);
		//					else
		//						it++;
		//				}


		//				//eliminate unselected stations
		//				order.resize(m_nbStations);
		//				m_locArray.resize(m_nbStations);
		//				for (size_t i = 0; i != order.size(); i++)
		//					m_locArray[i] = pWeatherDB->GetLocation(order[i]);
		//			}

		//		}
		//		else if (m_weatherGenerationMethod == WELL_DISTRIBUTED_STATIONS)
		//		{
		//			if (searchResult.size() > m_nbStations)
		//			{
		//				CLocationVector locIn = pWeatherDB->GetLocations(searchResult);
		//				GetClusterNode(pWeatherDB, m_nbStations, m_filter, m_year, locIn, m_locArray);
		//			}
		//		}
		//		else if (m_weatherGenerationMethod == COMPLETE_AND_DISTRIBUTED_STATIONS)
		//		{
		//			if (searchResult.size() > m_nbStations)
		//			{
		//				vector<size_t> priority;
		//				for (CSearchResultVector::iterator it = searchResult.begin(); it != searchResult.end(); it++)
		//				{
		//					CWVariablesCounter counter = pWeatherDB->GetWVariablesCounter(it->m_index, m_year);
		//					if (m_filter.any())
		//					{
		//						for (size_t v = 0; v < NB_VAR_H; v++)
		//							if (!m_filter[v])
		//								counter[v] = CCountPeriod();
		//					}

		//					priority.push_back(counter.GetSum());
		//				}

		//				CLocationVector locIn = pWeatherDB->GetLocations(searchResult);
		//				//éliminate unselected stations
		//				GenerateWellDistributedStation(m_nbStations, priority, locIn, m_locArray, progressDlg.GetCallback());
		//			}
		//		}

		//		pWeatherDB->Close();
		//	}
		//}



		return msg;
	}
	//ERMsg CLOCGeneratorDlg::GenerateFromNothing()
	//{
	//	ERMsg msg;
	//
	//	msg = VerifyParameter();
	//	if( msg)
	//	{
	//		
	//
	//		//CGeoRect rect( 
	//		//	UtilWin::GetDegDec(m_lonDeg1, m_lonMin1),
	//		//	UtilWin::GetDegDec(m_latDeg1, m_latMin1), 
	//		//	UtilWin::GetDegDec(m_lonDeg2, m_lonMin2),
	//		//	UtilWin::GetDegDec(m_latDeg2, m_latMin2),
	//		//	CProjection::GEO);
	//		
	//		m_rect.NormalizeRect();
	//
	//		if( !m_rect.IsRectEmpty() )
	//		{
	//			TGenerationMethod method  = GetGenerationMethod();
	//			bool bExp = (m_exposition != NONE); 
	//
	//			if( method == REGULAR)
	//			{
	//				int expoFactor = m_genType==0?1:30;
	//				m_locArray.GenerateRegularGrid(m_rect, m_nbPointLat, m_nbPointLon,
	//				m_elevMin, m_elevMax, bExp, expoFactor);
	//			}
	//			else
	//			{
	//				ASSERT( method == RANDOM);
	//				int expoFactor = m_genType==0?1:30;
	//				m_locArray.GenerateRandomGrid(m_rect, m_nbPoint,
	//				m_elevMin, m_elevMax, bExp, expoFactor);
	//			}
	//		}
	//		else
	//		{
	//			msg.asgType( ERMsg::ERREUR );
	//
	//			CString error;
	//			error.LoadString(IDS_SIM_GENLOC_BADCORNER);
	//			msg.ajoute(error);
	//		}
	//	}
	//	
	//	return msg;
	//}

	ERMsg CLOCGeneratorDlg::VerifyParameter()
	{
		ERMsg msg;

		switch (GetGenerationMethod())
		{
		case REGULAR:
			if (m_nbPointLat <= 0 || m_nbPointLon <= 0)
			{
				msg.ajoute(GetString(IDS_SIM_GENLOC_BADDENSITY));
				m_nbPointLatCtrl.SetFocus();
			}
			//if( m_nbPointLat*m_nbPointLon > kMaxPoint)
			//{
			//	msg.asgType( ERMsg::ERREUR );
			//	CString error;
			//	error.LoadString(IDS_SIM_GENLOC_TOMANYPOINT);
			//	msg.ajoute(error);
			//	
			//	m_nbPointLatCtrl.SetFocus();
			//}
			break;

		case RANDOM:
		{
			if (m_nbPoint <= 0)
			{
				msg.ajoute(GetString(IDS_SIM_GENLOC_BADDENSITY));
				m_nbPointCtrl.SetFocus();
			}

			//if(m_nbPoint > kMaxPoint)
			//{
			//	msg.asgType( ERMsg::ERREUR );
			//	CString error;
			//	error.LoadString(IDS_SIM_GENLOC_TOMANYPOINT);
			//	msg.ajoute(error);
			//	
			//	m_nbPointCtrl.SetFocus();
			//	
			//}
		}
		break;
		default: ASSERT(false);
		}

		return msg;
	}


	void CLOCGeneratorDlg::UpdateCtrl()
	{
		TGenerationFrom generateFrom = GetGenerateFromType();
		bool bEnableWeather = generateFrom == FROM_WEATHER;

		//Generate from station
		TWeatherStation stationType = GetWeatherStationType();
		bool bNormal = bEnableWeather && stationType == NORMAL;
		bool bDaily = bEnableWeather && stationType == DAILY;
		bool bHourly = bEnableWeather && stationType == HOURLY;

		GetStaticCtrl(1).EnableWindow(bEnableWeather);
		GetWeatherStationTypeCtrl().EnableWindow(bEnableWeather);
		GetStaticCtrl(2).EnableWindow(bEnableWeather);
		GetNormalDBNameCtrl().ShowWindow(stationType == NORMAL ? SW_SHOW : SW_HIDE);
		GetDailyDBNameCtrl().ShowWindow(stationType == DAILY ? SW_SHOW : SW_HIDE);
		GetHourlyDBNameCtrl().ShowWindow(stationType == HOURLY ? SW_SHOW : SW_HIDE);


		GetNormalDBNameCtrl().EnableWindow(bNormal);
		GetDailyDBNameCtrl().EnableWindow(bDaily);
		GetHourlyDBNameCtrl().EnableWindow(bHourly);
		GetStaticCtrl(3).EnableWindow(bDaily || bHourly);
		m_yearCtrl.EnableWindow(bDaily || bHourly);
		GetStaticCtrl(4).EnableWindow(bEnableWeather);
		GetFilterCtrl().EnableWindow(bEnableWeather);


		//generate from DEM
		TGenerationMethod method = GetGenerationMethod();
		bool bEnableDEM = generateFrom == FROM_DEM;
		bool bRegular = bEnableDEM && method == REGULAR;
		bool bRandom = bEnableDEM && method == RANDOM;

		GetStaticCtrl(5).EnableWindow(true);
		GetGenerationMethodCtrl().EnableWindow(bEnableDEM);
		GetStaticCtrl(6).EnableWindow(bEnableDEM);
		GetDEMNameCtrl().EnableWindow(bEnableDEM);
		GetDEMBrowseCtrl().EnableWindow(bEnableDEM);
		GetStaticCtrl(7).EnableWindow(bRandom || GetWeatherGenerationMethod() != ALL_STATIONS);
		m_nbPointCtrl.EnableWindow(bRandom);
		GetStaticCtrl(8).EnableWindow(bRegular);
		m_nbPointLatCtrl.EnableWindow(bRegular);
		GetStaticCtrl(9).EnableWindow(bRegular);
		m_nbPointLonCtrl.EnableWindow(bRegular);
		GetStaticCtrl(10).EnableWindow(bRandom);
		GetDlgItem(IDC_GENLOC_FACTOR)->EnableWindow(bRandom);
		GetDlgItem(IDC_GENLOC_EXTREM)->EnableWindow(bRandom);


		GetGenerationMethodCtrl().ShowWindow(bEnableDEM ? SW_SHOW : SW_HIDE);
		m_nbPointCtrl.ShowWindow(bEnableDEM ? SW_SHOW : SW_HIDE);
		GetWeatherGenerationMethodCtrl().ShowWindow(bEnableWeather ? SW_SHOW : SW_HIDE);
		m_nbStationCtrl.ShowWindow(bEnableWeather ? SW_SHOW : SW_HIDE);
		m_nbStationCtrl.EnableWindow(GetWeatherGenerationMethod() != ALL_STATIONS);

		m_exposureCtrl.EnableWindow(bEnableDEM);
		bool bGenarateExpo = bEnableDEM && GetCheckedRadioButton(IDC_GENLOC_NO_EXPOSITION, IDC_GENLOC_EXPOSITION) == IDC_GENLOC_EXPOSITION;

		GetUniformExpositionCtrl().EnableWindow(bGenarateExpo);
		GetNormalExpositionCtrl().EnableWindow(bGenarateExpo);
	}

	void CLOCGeneratorDlg::OnSelectDEM()
	{
		CDBManagerDlg dlg(this, CDBManagerDlg::MAP_INPUT);
		dlg.m_mapPage.m_lastDEMName = GetDEMNameCtrl().GetWindowText();

		if (dlg.DoModal() == IDOK)
		{
			FillDEMList();
			CString DEMName = dlg.m_mapPage.m_lastDEMName;
			int sel = GetDEMNameCtrl().SelectStringExact(-1, DEMName);
		}

	}




	void CLOCGeneratorDlg::FillDEMList(void)
	{
		WBSF::StringVector mapList = WBSF::GetFM().MapInput().GetFilesList();
		GetDEMNameCtrl().FillList(mapList);
	}

	void CLOCGeneratorDlg::FillNormalDBList()
	{
		WBSF::StringVector DBList = WBSF::GetFM().Normals().GetFilesList();
		GetNormalDBNameCtrl().FillList(DBList);
	}

	void CLOCGeneratorDlg::FillDailyDBList()
	{
		WBSF::StringVector DBList = WBSF::GetFM().Daily().GetFilesList();
		GetDailyDBNameCtrl().FillList(DBList);
	}

	void CLOCGeneratorDlg::FillHourlyDBList()
	{
		WBSF::StringVector DBList = WBSF::GetFM().Hourly().GetFilesList();
		GetHourlyDBNameCtrl().FillList(DBList);
	}

	void CLOCGeneratorDlg::OnGenerateFromChange()
	{
		TGenerationFrom generateFrom = GetGenerateFromType();

		if (generateFrom != m_generateFrom)
		{
			if (m_generateFrom == FROM_DEM &&
				GetCheckedRadioButton(IDC_GENLOC_NO_EXPOSITION, IDC_GENLOC_EXPOSITION) == IDC_GENLOC_EXPOSITION_FROM_DEM)
			{
				CheckRadioButton(IDC_GENLOC_NO_EXPOSITION, IDC_GENLOC_EXPOSITION, IDC_GENLOC_EXPOSITION);
			}
			m_generateFrom = generateFrom;

			UpdateCtrl();
		}
	}


	void CLOCGeneratorDlg::OnExpoTypeChange()
	{
		UpdateCtrl();
	}


	void CLOCGeneratorDlg::OnDestroy()
	{
		CRegistry option("", "Generate LOC Dlg");

		//option.SetCurrentProfile();
		option.SetValue("GeoRect", m_rect);
		option.SetValue("exposition", m_exposition);
		option.SetValue("Generate from", m_generateFrom);
		option.SetValue("Generation method", m_generationMethod);
		option.SetValue("WeatherGenerationMethod", m_weatherGenerationMethod);
		option.SetValue("Weather station", m_weatherStation);
		option.SetValue("Filter", m_filter);
		option.SetValue("year", m_year);
		option.SetValue("nb gen", m_nbPoint);
		option.SetValue("nb point lat", m_nbPointLat);
		option.SetValue("nb point lon", m_nbPointLon);
		option.SetValue("NbStations", m_nbStations);
		option.SetValue("bExtrem", m_bElevExtrem);
		option.SetValue("factor", m_factor);
		option.SetValue("gen expo type", m_genType);
		option.SetValue("DEM Name", UtilWin::ToUTF8(m_DEMName));
		option.SetValue("NormalDBName", UtilWin::ToUTF8(m_normalDBName));
		option.SetValue("DailyDBName", UtilWin::ToUTF8(m_dailyDBName));
		option.SetValue("HourlyDBName", UtilWin::ToUTF8(m_hourlyDBName));
		option.SetValue("UseBoundingBox", m_useBoundingBox);

		CDialog::OnDestroy();
	}

}