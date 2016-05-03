#include "StdAfx.h"
#include "ClipWeather.h"
#include "Basic/DailyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/HourlyDatabase.h"
//#include "FileManagerRes.h"
//#include "SYShowMessage.h"
//#include "BasicRes.h"
//#include "CommonRes.h"
//#include "mappingRes.h"

#include "basic/UtilStd.h"
#include "Geomatic/ShapefileBase.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//*********************************************************************
	const char* CClipWeather::ATTRIBUTE_NAME[] = { "InputFilepath", "OutputFilepath", "FirstYear", "LastYear", "IncludeID", "ExcludeID", "BoundingBox", "shapefile", "LocFilepath", "Variables" };
	const size_t CClipWeather::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_STRING, T_STRING, T_STRING, T_STRING, T_GEORECT, T_FILEPATH, T_FILEPATH, T_STRING_SELECT };
	const UINT CClipWeather::ATTRIBUTE_TITLE_ID = IDS_TOOL_CROP_DB_P;
	const UINT CClipWeather::DESCRIPTION_TITLE_ID = ID_TASK_CROP_DB;

	const char* CClipWeather::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ClipWeather";  return THE_CLASS_NAME; }
	CTaskBase::TType CClipWeather::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CClipWeather::CLASS_NAME(), (createF)CClipWeather::create);
	

	CClipWeather::CClipWeather(void)
	{}

	CClipWeather::~CClipWeather(void)
	{}

	
	std::string CClipWeather::Option(size_t i)const
	{
	
		string str;
		switch (i)
		{
		case INPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_WEATHER); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_WEATHER); break;
		case LOC_FILEPATH:		str = GetString(IDS_STR_FILTER_LOC); break;
		case SHAPEFILE:			str = GetString(IDS_STR_FILTER_SHAPEFILE); break;
		case VARIABLES:
		{
			
			for (TVarH v = H_TAIR; v <NB_VAR_H; v++)
				str += string("|") + GetVariableAbvr(v) + "=" + GetVariableTitle(v);
			break;
		}
		};


		return str;
	}

	std::string CClipWeather::Default(size_t i)const
	{
		std::string str;

		switch (i)
		{
		//BOUNDINGBOX, 
		case FIRST_YEAR:	str = "1901"; break;
		case LAST_YEAR:		str = "2100"; break;
		};

		return str;
	}


	ERMsg CClipWeather::Execute(CCallback& callback)
	{
		ERMsg msg;

		CGeoRect boundingBox;


		if (boundingBox.IsRectNormal())
		{
			string inputFilePath = TrimConst(Get(INPUT_FILEPATH));
			string outputFilePath = TrimConst(Get(OUTPUT_FILEPATH));
			string extention = GetFileExtension(inputFilePath);
			SetFileExtension(outputFilePath, extention);
			
			if (!IsEqualNoCase(outputFilePath, inputFilePath))
			{
				if (IsEqualNoCase(extention, CNormalsDatabase().GetDatabaseExtension()))
				{
					msg = CNormalsDatabase().DeleteDatabase(outputFilePath, callback);
					if (msg)
						msg = ExecuteNormals(inputFilePath, outputFilePath, callback);
				}
				else if (IsEqualNoCase(extention, CDailyDatabase().GetDatabaseExtension()))
				{
					msg = CDailyDatabase().DeleteDatabase(outputFilePath, callback);
					if (msg)
						msg = ExecuteDaily(inputFilePath, outputFilePath, callback);
				}
				else if (IsEqualNoCase(extention, CHourlyDatabase().GetDatabaseExtension()))
				{
					msg = CHourlyDatabase().DeleteDatabase(outputFilePath, callback);
					if (msg)
						msg = ExecuteHourly(inputFilePath, outputFilePath, callback);
				}
				else
				{
					msg.ajoute("Bad database extension");
				}
			}
			else
			{
				msg.ajoute(GetString(IDS_BSC_SAME_NAMES));
			}
		}
		else
		{
			msg.ajoute("bounding box coordinates are not valid. Verify that minimums are lower than maximums");
		}

		return msg;
	}


	ERMsg CClipWeather::ExecuteHourly(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CClipWeather::ExecuteDaily(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		string shapeFilePath = TrimConst(Get(SHAPEFILE));
		string locFilePath = TrimConst(Get(LOC_FILEPATH));
		StringVector includeIds(TrimConst(Get(INCLUDE_IDS)), "|;");
		StringVector excludeIds(TrimConst(Get(EXCLUDE_IDS)), "|;");
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		CGeoRect boundingBox;

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);
		callback.AddMessage("");


		//Get the data for each station
		CDailyDatabase inputDailyDB;
		CDailyDatabase outputDailyDB;
		CShapeFileBase shapefile;
		CLocationVector locInclude;
		CLocationVector locExclude;

		if (msg )
			msg += inputDailyDB.Open(GetAbsoluteFilePath(inputFilePath), CDailyDatabase::modeRead, callback);
		
		if (msg)
			msg += outputDailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);

		if (msg && !shapeFilePath.empty())
			msg += shapefile.Read(GetAbsoluteFilePath(shapeFilePath));

		if (msg && !locFilePath.empty())
			msg += locInclude.Load(GetAbsoluteFilePath(locFilePath));

		if (!msg)
			return msg;

		if (!includeIds.empty())
		{
			for (size_t i = 0; i < includeIds.size(); i++)
				locInclude.push_back(CLocation(includeIds[i], includeIds[i], 0, 0, 0));
		}
			
		
		if (!excludeIds.empty())
		{
			for (size_t i = 0; i < excludeIds.size(); i++)
				locExclude.push_back(CLocation(excludeIds[i], excludeIds[i], 0, 0, 0));
		}


		callback.PushTask(GetString(IDS_CLIP_WEATHER), inputDailyDB.size());
		//callback.SetNbStep();


		int nbStationAdded = 0;
		for (size_t i = 0; i < inputDailyDB.size() && msg; i++)
		{
			const CLocation& location = inputDailyDB[i];

			set<int> years = inputDailyDB.GetYears(i);
			for (set<int>::iterator it = years.begin(); it != years.end();)
			{
				if (*it >= firstYear&&*it <= lastYear)
					it++;
				else
					it = years.erase(it);
			}


			//CGeoPoint pt(stationHead.m_lon, stationHead.m_lat, PRJ_WGS_84 );
			bool bRect = boundingBox.IsRectNull() || boundingBox.PtInRect(location);
			bool bShape = shapefile.GetNbShape() == 0 || shapefile.IsInside(location);
			bool bLocInclude = locInclude.size() == 0 || locInclude.FindByID(location.m_ID) != NOT_INIT;
			bool bLocExclude = locExclude.FindByID(location.m_ID) == NOT_INIT;
			if (bRect&&bShape&&bLocInclude&&!bLocExclude&&years.size() > 0)
			{
				CWeatherStation station;
				inputDailyDB.Get(station, i, years);

				msg = outputDailyDB.push_back(station);
				if (msg)
					nbStationAdded++;

			}

			msg += callback.StepIt();
		}

		outputDailyDB.Close();
		callback.PopTask();

		//Create optimization files
		if (msg)
		{
			msg = outputDailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
			if (msg)
				outputDailyDB.Close();
		}

		callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);



		return msg;
	}

	//***********************************************************************************
	ERMsg CClipWeather::ExecuteNormals(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		string shapeFilePath = TrimConst(Get(SHAPEFILE));
		string locFilePath = TrimConst(Get(LOC_FILEPATH));
		StringVector includeIds(TrimConst(Get(INCLUDE_IDS)), "|;");
		StringVector excludeIds(TrimConst(Get(EXCLUDE_IDS)), "|;");
		CGeoRect boundingBox;

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);
		callback.AddMessage("");


		//Get the data for each station
		CNormalsDatabase inputDB;
		CNormalsDatabase outputDB;
		CShapeFileBase shapefile;
		CLocationVector loc;
		CLocationVector locInclude;
		CLocationVector locExclude;


		if (!shapeFilePath.empty())
			msg += shapefile.Read(shapeFilePath);

		if (!locFilePath.empty())
			msg += loc.Load(locFilePath);

		msg += inputDB.Open(inputFilePath, CNormalsDatabase::modeRead, callback);
		msg += outputDB.Open(outputFilePath, CNormalsDatabase::modeEdit, callback);

		if (!msg)
			return msg;

		if (!includeIds.empty())
		{
			for (size_t i = 0; i < includeIds.size(); i++)
				locInclude.push_back(CLocation(includeIds[i], includeIds[i], 0, 0, 0));
		}


		if (!excludeIds.empty())
		{
			for (size_t i = 0; i < excludeIds.size(); i++)
				locExclude.push_back(CLocation(excludeIds[i], excludeIds[i], 0, 0, 0));
		}
		
		
		callback.PushTask("Merge Normals", inputDB.size());

		int nbStationAdded = 0;
		for (size_t i = 0; i < inputDB.size() && msg; i++)
		{
			CNormalsStation station;
			inputDB.Get(station, i);

			CGeoPoint pt(station.m_lon, station.m_lat, PRJ_WGS_84);
			bool bRect = boundingBox.IsRectNull() || boundingBox.PtInRect(station);
			bool bShape = shapefile.GetNbShape() == 0 || shapefile.IsInside(pt);
			bool bLocInclude = locInclude.size() == 0 || locInclude.FindByID(station.m_ID) != NOT_INIT;
			bool bLocExclude = locExclude.FindByID(station.m_ID) == NOT_INIT;
			if (bRect&&bShape&&bLocInclude&&!bLocExclude)

			{
				msg = outputDB.Add(station);
				if (msg)
					nbStationAdded++;
			}

			msg += callback.StepIt();
		}

		outputDB.Close();
		callback.PopTask();


		if (msg)
		{
			msg = outputDB.Open(outputFilePath, CNormalsDatabase::modeRead, callback);
			if (msg)
				outputDB.Close();
		}

		callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);

		
		return msg;
	}

	
}