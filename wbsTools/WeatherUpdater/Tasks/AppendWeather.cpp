#include "StdAfx.h"
#include "AppendWeather.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "basic/UtilStd.h"
#include "Geomatic/ShapefileBase.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
namespace WBSF
{

	//*********************************************************************
	const char* CAppendWeather::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "InputFilepath1", "InputFilepath2", "OutputFilePath" };
	const size_t CAppendWeather::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH };
	const UINT CAppendWeather::ATTRIBUTE_TITLE_ID = IDS_TOOL_APPEND_DB_P;
	const UINT CAppendWeather::DESCRIPTION_TITLE_ID = ID_TASK_APPEND_DB;
	
	const char* CAppendWeather::CLASS_NAME(){ static const char* THE_CLASS_NAME = "AppendWeather";  return THE_CLASS_NAME; }
	CTaskBase::TType CAppendWeather::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CAppendWeather::CLASS_NAME(), (createF)CAppendWeather::create);

	CAppendWeather::CAppendWeather(void)
	{}

	CAppendWeather::~CAppendWeather(void)
	{}


	std::string CAppendWeather::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case INPUT_FILEPATH_1:	
		case INPUT_FILEPATH_2:	
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_OBSERVATION); break;
		};


		return str;
	}

	



	ERMsg CAppendWeather::Execute(CCallback& callback)
	{
		ERMsg msg;
		
		string inputFilePath1 = Get(INPUT_FILEPATH_1);
		string inputFilePath2 = Get(INPUT_FILEPATH_2);
		string outputFilePath = Get(OUTPUT_FILEPATH);

		string extention1 = GetFileExtension(inputFilePath1);
		string extention2 = GetFileExtension(inputFilePath2);

		if (IsEqualNoCase(extention1, ".NormalsStation") && IsEqualNoCase(extention2, ".NormalsStation"))
		{
			msg.ajoute("It's not possible to append normals stations");
			//msg = ExecuteNormal(callback);
		}
		else if (IsEqualNoCase(extention1, ".DailyStations") && IsEqualNoCase(extention2, ".DailyStations"))
		{
			SetFileExtension(outputFilePath, ".DailyStations");

			msg = CDailyDatabase().DeleteDatabase(outputFilePath, callback);
			if (msg)
			{
				CDailyDatabase DB;
				msg = DB.Open(outputFilePath, CDailyDatabase::modeWrite, callback);
				if (msg)
				{
					msg = DB.AppendDatabase(inputFilePath1, inputFilePath2, callback);
					msg += DB.Close();
					if (msg)
					{
						msg = DB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
						if (msg)
							DB.Close();
					}
				}
			}
		}
		else if (IsEqualNoCase(extention1, ".HourlyStations") && IsEqualNoCase(extention2, ".HourlyStations"))
		{
			SetFileExtension(outputFilePath, ".HourlyStations");

			msg = CHourlyDatabase().DeleteDatabase(outputFilePath, callback);
			if (msg)
			{
				CHourlyDatabase DB;
				msg = DB.Open(outputFilePath, CHourlyDatabase::modeWrite, callback);
				if (msg)
				{
					msg = DB.AppendDatabase(inputFilePath1, inputFilePath2, callback);
					msg += DB.Close();
					if (msg)
					{
						msg = DB.Open(outputFilePath, CHourlyDatabase::modeRead, callback);
						if (msg)
							DB.Close();
					}
				}
			}
		}
		else
		{
			msg.ajoute("Database must be of the same type");
		}

		return msg;
	}
}
