#include "stdafx.h"
#include <boost/functional/factory.hpp>
#include <boost/bind.hpp>

#include "ConvertDB.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"

#include "Basic/UtilStd.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "WeatherBasedSimulationString.h"
#include "../resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//*********************************************************************
	//*********************************************************************
	
	const char* CConvertDB::ATTRIBUTE_NAME[] = { "Direction", "InputFilepath", "OutputFilepath" };
	const size_t CConvertDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_COMBO_INDEX, T_FILEPATH, T_FILEPATH };
	const UINT CConvertDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CONVERT_DB_P;
	const UINT CConvertDB::DESCRIPTION_TITLE_ID = ID_TASK_CONVERT_DB;

	const char* CConvertDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ConvertDB";  return THE_CLASS_NAME; }
	CTaskBase::TType CConvertDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CConvertDB::CLASS_NAME(), (createF)CConvertDB::create);

	CConvertDB::CConvertDB(void)
	{
	}
	
	CConvertDB::~CConvertDB(void)
	{
	}

	

	std::string CConvertDB::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case CONVERT_TYPE:		str = GetString(IDS_CONVERT_DIRECTION); break;
		case INPUT_FILEPATH:str = GetString(IDS_STR_FILTER_WEATHER); break;
		case OUTPUT_FILEPATH:str = GetString(IDS_STR_FILTER_WEATHER); break;
		};
		return str;
	}

	std::string CConvertDB::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case CONVERT_TYPE: str = ToString(TO_BIOSIM_11); break;
		}

		return str;
	}


	ERMsg CConvertDB::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t type = ToSizeT(Get(CONVERT_TYPE));
		string inputFilepath = GetAbsoluteFilePath(Get(INPUT_FILEPATH));
		string outputFilepath = GetAbsoluteFilePath(Get(OUTPUT_FILEPATH));
		

		if (type == TO_BIOSIM_11 || type == TO_BIOSIM_10)
		{
			string extentionIn = GetFileExtension(inputFilepath);
			string extentionOut = GetFileExtension(extentionIn);
			if (IsEqual(extentionIn, CNormalsDatabase::DATABASE_EXT) || IsEqual(extentionIn, ".Normals"))
				extentionOut = (type == TO_BIOSIM_11) ? CNormalsDatabase::DATABASE_EXT : ".Normals";

			if (IsEqual(extentionIn, CDailyDatabase::DATABASE_EXT) || IsEqual(extentionIn, ".DailyStations"))
				extentionOut = (type == TO_BIOSIM_11) ? CDailyDatabase::DATABASE_EXT : ".DailyStations";

			SetFileExtension(outputFilepath, extentionOut);


			if (IsEqual(outputFilepath, inputFilepath))
			{
				msg.ajoute(GetString(IDS_BSC_SAME_NAMES));
				return msg;
			}

			CreateMultipleDir(GetPath(outputFilepath));

			if (IsEqualNoCase(extentionIn, ".Normals") || IsEqualNoCase(extentionIn, CNormalsDatabase::DATABASE_EXT))
			{
				if (type == TO_BIOSIM_11)
				{
					msg = CNormalsDatabase::DeleteDatabase(outputFilepath, callback);
					if (msg)
						msg = CNormalsDatabase::v6_to_v7(inputFilepath, outputFilepath, callback);
				}
				else
				{
					if (FileExists(outputFilepath))
						msg = WBSF::RemoveFile(outputFilepath);

					if (msg)
						msg = CNormalsDatabase::v7_to_v6(inputFilepath, outputFilepath, callback);
				}

			}
			else if (IsEqualNoCase(extentionIn, ".DailyStations") || IsEqualNoCase(extentionIn, CDailyDatabase::DATABASE_EXT))
			{
				msg = CDailyDatabase::DeleteDatabase(outputFilepath, callback);
				if (msg)
				{
					if (type == TO_BIOSIM_11)
						msg = CDailyDatabase::v3_to_v4(inputFilepath.c_str(), outputFilepath.c_str(), callback);
					else
						msg = CDailyDatabase::v4_to_v3(inputFilepath.c_str(), outputFilepath.c_str(), callback);
				}
			}
			else
			{
				msg.ajoute("Unknown database type. Verify file extension.");
			}
		}
		else if (type == HOURLY_TO_DAILY)
		{
			string extentionIn = GetFileExtension(inputFilepath);
			string extentionOut = GetFileExtension(outputFilepath);

			
			if (IsEqual(extentionIn, CHourlyDatabase::DATABASE_EXT) && IsEqual(extentionOut, CDailyDatabase::DATABASE_EXT))
			{
				msg = CDailyDatabase::DeleteDatabase(outputFilepath, callback);
				if (msg)
				{
					CHourlyDatabase input;
					CDailyDatabase output;
					msg = input.Open(inputFilepath, CHourlyDatabase::modeRead, callback);
					if(msg)
						msg = output.Open(outputFilepath, CHourlyDatabase::modeWrite, callback);
					if (msg)
					{
						callback.PushTask("Convert " + to_string(input.size()) + " stations", input.size());
						for (size_t i = 0; i < input.size()&&msg; i++)
						{
							CWeatherStation station;
							msg += input.Get(station, i);
							station.GetStat(H_TAIR);//compute daily stat
							station.SetHourly(false);//remove hourly values

							if (msg)
								msg +=output.Add(station);

							msg += callback.StepIt();
						}

						input.Close();
						msg += output.Close(true, callback);
						if (msg)
						{
							CDailyDatabase test;
							msg = test.Open(outputFilepath, CHourlyDatabase::modeRead, callback);
							test.Close();
						}
						callback.PopTask();
					}
				}
			}
			else
			{
				msg.ajoute("Input file must be hourly database (.HourlyDB) and output file must be daily database (.DailyDB) ");
			}
			
		}

		return msg;
	}


}
