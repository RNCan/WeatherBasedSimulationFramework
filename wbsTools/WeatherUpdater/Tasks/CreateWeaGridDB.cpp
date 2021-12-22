#include "StdAfx.h"
#include "CreateWeaGridDB.h"
#include "Basic/WeatherStation.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/Timer.h"
#include "Basic/CSV.h"
#include "Basic/OpenMP.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{
	//*********************************************************************
	const char* CCreateWeatherGridDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GribsFilePath", "LocationsFilePath", "OutputFilePath", "Variables", "NbPoints", "Incremental" };
	const size_t CCreateWeatherGridDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_STRING_SELECT, T_STRING, T_BOOL };
	const UINT CCreateWeatherGridDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_WEAGRID_P;
	const UINT CCreateWeatherGridDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_WEAGRID;


	const char* CCreateWeatherGridDB::CLASS_NAME() { static const char* THE_CLASS_NAME = "CreateGridDB";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateWeatherGridDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateWeatherGridDB::CLASS_NAME(), CCreateWeatherGridDB::create);


	CCreateWeatherGridDB::CCreateWeatherGridDB(void)
	{}

	CCreateWeatherGridDB::~CCreateWeatherGridDB(void)
	{}

	std::string CCreateWeatherGridDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_GRIBS); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_OBSERVATION); break;
		case LOCATIONS_FILEPATH:str = GetString(IDS_STR_FILTER_LOC); break;
		case VARIABLES:			str = "Tmin|Tair|Tmax|Prcp|Tdew|RelH|WndS|WndD|SRad|Pres|Snow|SnDh|SWE|Wnd2"; break;
		case NB_POINTS:			str = "0"; break;//at the point itself
		};
		
		return str;
	}

	std::string CCreateWeatherGridDB::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case NB_POINTS:		str = "4"; break;
		};

		return str;
	}
	bool GDALGetCenter(GDALDataset* poDS, double* centerLonLat)
	{
		char* pszPrj;
		double adfGeoTransform[6];
		int xSize, ySize;
		double xCenter, yCenter;
		double lon, lat;

		OGRSpatialReference oSourceSRS, oTargetSRS;
		OGRCoordinateTransformation* poCT;

		if (poDS == NULL)
			return false;

		xSize = poDS->GetRasterXSize();
		ySize = poDS->GetRasterYSize();

		if (poDS->GetGeoTransform(adfGeoTransform) != CE_None)
			return false;

		if (poDS->GetProjectionRef() == NULL)
			return false;
		else
			pszPrj = (char*)poDS->GetProjectionRef();

		oSourceSRS.importFromWkt(&pszPrj);
		oTargetSRS.SetWellKnownGeogCS("WGS84");

		poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS);
		if (poCT == NULL)
			return false;

		xCenter = xSize / 2;
		yCenter = ySize / 2;

		lon = adfGeoTransform[0] + adfGeoTransform[1] * xCenter
			+ adfGeoTransform[2] * yCenter;

		lat = adfGeoTransform[3] + adfGeoTransform[4] * xCenter
			+ adfGeoTransform[5] * yCenter;

		if (!poCT->Transform(1, &lon, &lat)) {
			OGRCoordinateTransformation::DestroyCT(poCT);
			return false;
		}

		centerLonLat[0] = lon;
		centerLonLat[1] = lat;

		OGRCoordinateTransformation::DestroyCT(poCT);
		return true;
	}
	ERMsg CCreateWeatherGridDB::Execute(CCallback& callback)
	{
		ERMsg msg;

		GDALSetCacheMax64(128 * 1024 * 1024);
		

		
		CPLStringList paths ( OSRGetPROJSearchPaths() );
		

		string inputFilePath = Get(INPUT_FILEPATH);
		if (inputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		string outputFilePath = Get(OUTPUT_FILEPATH);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		msg = CreateMultipleDir(GetPath(outputFilePath));

		//SetFileExtension(outputFilePath, (outputType == OT_HOURLY) ? CHourlyDatabase::DATABASE_EXT : CDailyDatabase::DATABASE_EXT);
		SetFileExtension(outputFilePath, CHourlyDatabase::DATABASE_EXT);
		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		


		CGribsMap gribs;
		msg += gribs.load(inputFilePath);


		CLocationVector locations;
		if (msg)
			msg += locations.Load(Get(LOCATIONS_FILEPATH));



		CSfcGribDatabase grib;
		grib.m_nb_points = as<size_t>(NB_POINTS);
		grib.m_bIncremental = as<bool>(INCREMENTAL);
		grib.m_variables = GetVariables(Get(VARIABLES));
		grib.m_nbMaxThreads = omp_get_num_procs();

		if (!grib.m_bIncremental)
		{
			//delete incremental file
			msg += CIncementalDB::Delete(outputFilePath);
			
			//delete database
			msg += grib.DeleteDatabase(outputFilePath, callback);
		}

		if(msg)
			msg += grib.Open(outputFilePath, CDailyDatabase::modeWrite);

		
		if (msg)
		{ 
			msg = grib.Update(gribs, locations, callback);
			if (msg)
				msg += grib.Close(true, callback);

			if (msg)
			{
				//open for verification
				msg += grib.Open(outputFilePath, CDailyDatabase::modeRead, callback);
				grib.Close();
			}
		}

		return msg;
	}

	GribVariables CCreateWeatherGridDB::GetVariables(string str)
	{
		
		GribVariables variables;
		if (str.empty())
		{
			variables.set();
		}
		else
		{
			variables = CSfcGribDatabase::get_var(CWVariables(str));

			/*StringVector vars(str, ",;| ");
			for (size_t v = 0; v < vars.size(); v++)
			{
				TVarH var = HOURLY_DATA::GetVariableFromName(vars[v], true);
				if (var != SKIP)
					variables.set(var);
			}*/
		}

		return variables;
	}

	/*ERMsg CCreateWeatherGridDB::ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;

		string str = Get(VARIABLES);
		GribVariable variables = GetVariables(str);

		CSfcDatasetCached sfcDS;
		sfcDS.set_variables(variables);

		msg = sfcDS.open(file_path, true);
		if (msg)
		{
			CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, sfcDS.GetPrjID());
			for (size_t i = 0; i < stations.size() && msg; i++)
			{
				CGeoPoint pt = stations[i];
				pt.Reproject(GEO_2_WEA);
				if (sfcDS.GetExtents().IsInside(pt))
				{
					CHourlyData& data = stations[i].GetHour(TRef);
					sfcDS.get_weather(pt, data);

					msg += callback.StepIt();
				}
			}

			sfcDS.close();
		}

		return msg;
	}*/
	//ERMsg CCreateWeatherGridDB::CreateDatabase(const std::string& outputFilePath, CTaskPtr& pTask, CTaskPtr& pForecastTask, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	CTimer timer(true);
	//	CTimer timerRead;
	//	CTimer timerWrite;

	//	
	//	callback.AddMessage(GetString(IDS_CREATE_DB));
	//	callback.AddMessage(outputFilePath, 1);

	//	//Get the data for each station
	//	CDailyDatabase dailyDB;
	//	msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);
	//	if (!msg)
	//		return msg;

	//	int nbStationAdded = 0;

	//	StringVector stationList;
	//	msg = pTask->GetStationList(stationList, callback);

	//	if (msg)
	//	{
	//		callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(stationList.size()) + " stations)", stationList.size());


	//		for (size_t i = 0; i < stationList.size() && msg; i++)
	//		{
	//			CWeatherStation station;

	//			timerRead.Start();
	//			ERMsg messageTmp = pTask->GetWeatherStation(stationList[i], CTM(CTM::DAILY), station, callback);
	//			timerRead.Stop();

	//			if (messageTmp)
	//			{
	//				CleanSparse(station);

	//				if (station.HaveData())
	//				{
	//					string newName = dailyDB.GetUniqueName(station.m_name);
	//					if (newName != station.m_name)
	//					{
	//						station.m_name = newName;
	//						station.SetDataFileName("");
	//					}

	//					//Force write file name in the file
	//					station.SetDataFileName(station.GetDataFileName());
	//					station.UseIt(true);

	//					//Get forecast
	//					if (pForecastTask)
	//						msg += pForecastTask->GetWeatherStation("", CTM(CTM::DAILY), station, callback);
	//					

	//					timerWrite.Start();
	//					messageTmp = dailyDB.Add(station);
	//					timerWrite.Stop();

	//					if (messageTmp)
	//						nbStationAdded++;
	//				}
	//			}
	//			else
	//			{
	//				if (callback.GetUserCancel())
	//					msg += messageTmp;
	//			}

	//			if (!messageTmp)
	//				callback.AddMessage(messageTmp, 1);

	//			msg += callback.StepIt();

	//		}

	//		msg += dailyDB.Close();
	//		timer.Stop();
	//		callback.PopTask();


	//		if (msg)
	//		{
	//			msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
	//			dailyDB.Close();
	//		}

	//		if (msg)
	//		{
	//			callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);
	//			callback.AddMessage(FormatMsg(IDS_BSC_TIME_READ, SecondToDHMS(timerRead.Elapsed())));
	//			callback.AddMessage(FormatMsg(IDS_BSC_TIME_WRITE, SecondToDHMS(timerWrite.Elapsed())));
	//			callback.AddMessage(FormatMsg(IDS_BSC_TOTAL_TIME, SecondToDHMS(timer.Elapsed())));
	//		}


	//	}

	//	return msg;
	//}



	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW)
			s = SUM;

		return s;
	}


}