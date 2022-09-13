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
	const char* CCreateWeatherGridDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GribsFilePath", "LocationsFilePath", "OutputFilePath", "Variables", "NbPoints", "FirstYear", "LastYear", "Incremental" };
	const size_t CCreateWeatherGridDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_STRING_SELECT, T_STRING, T_STRING, T_STRING, T_BOOL };
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
		case FIRST_YEAR:	str = "2020"; break;
		case LAST_YEAR:		str = "2020"; break;
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

		
		


		CGribsMap gribs;
		msg += gribs.load(inputFilePath);



		CLocationVector locations;
		if (msg)
			msg += locations.Load(Get(LOCATIONS_FILEPATH));


		SetFileExtension(outputFilePath, (gribs.GetEntireTPeriod().GetTM()==CTM::HOURLY) ? CHourlyDatabase::DATABASE_EXT : CDailyDatabase::DATABASE_EXT);
		//SetFileExtension(outputFilePath, CHourlyDatabase::DATABASE_EXT);
		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);


		CSfcGribDatabase grib;
		grib.m_nb_points = as<size_t>(NB_POINTS);
		grib.m_bIncremental = as<bool>(INCREMENTAL);
		grib.m_variables = GetVariables(Get(VARIABLES));
		grib.m_period = CTPeriod(CTRef(as<int>(FIRST_YEAR), JANUARY, DAY_01, 0), CTRef(as<int>(LAST_YEAR), DECEMBER, DAY_31, 23)).Transform(gribs.GetEntireTPeriod());
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
				if (msg)
				{
					ASSERT(grib.GetDB() != nullptr);
					callback.AddMessage("Number of station in database: " + to_string(grib.GetDB()->size()));
				}
				
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

	

	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW)
			s = SUM;

		return s;
	}


}