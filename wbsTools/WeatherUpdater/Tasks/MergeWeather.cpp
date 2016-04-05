#include "StdAfx.h"
#include "MergeWeather.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/Statistic.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::WEATHER;

namespace WBSF
{


	//*********************************************************************
	const char* CMergeWeather::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "InputFilepath1", "InputFilepath2", "OutputFilepath", "Distance", "DeltaElev", "Type", "PriorityRule" };
	const size_t CMergeWeather::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_STRING, T_STRING, T_COMBO_POSITION, T_COMBO_POSITION };
	const UINT CMergeWeather::ATTRIBUTE_TITLE_ID = IDS_TOOL_MERGE_DATABASE_P;
	
	
	const char* CMergeWeather::CLASS_NAME(){ static const char* THE_CLASS_NAME = "MergeWeather";  return THE_CLASS_NAME; }
	CTaskBase::TType CMergeWeather::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CMergeWeather::CLASS_NAME(), CMergeWeather::create);

	CMergeWeather::CMergeWeather(void)
	{}

	
	CMergeWeather::~CMergeWeather(void)
	{}



	std::string CMergeWeather::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case INPUT_DB1:		str = GetString(IDS_STR_FILTER_WEATHER); break;
		case INPUT_DB2:		str = GetString(IDS_STR_FILTER_WEATHER); break;
		case OUTPUT_FILEPATH:  str = GetString(IDS_STR_FILTER_WEATHER); break;
		case MERGE_TYPE:	str = GetString(IDS_MERGE_TYPE); break;
		case PRIORITY_RULE: str = GetString(IDS_PRIORITY_RULES); break;
		};

		
		return str;
	}

	std::string CMergeWeather::Default(size_t i)const
	{
		std::string str;

		switch (i)
		{
		case DISTANCE:		str = "5000"; break;
		case DELTA_ELEV:	str = "50"; break;
		case MERGE_TYPE:	str = ToString(MERGE_FROM_MEAN); break;
		case PRIORITY_RULE: str = ToString(LARGEST_DATA); break;
		};

		return str;
	}



	ERMsg CMergeWeather::Execute(CCallback& callback)
	{
		ERMsg msg;

		
		string inputFilePath1 = TrimConst(Get(INPUT_DB1));
		string inputFilePath2 = TrimConst(Get(INPUT_DB2));
		string outputFilePath = TrimConst(Get(OUTPUT_FILEPATH));
		string extention1 = GetFileExtension(inputFilePath1);
		string extention2 = GetFileExtension(inputFilePath2);

		if (IsEqual(extention1, extention2))
		{
			if (!IsEqual(outputFilePath, inputFilePath1) &&
				!IsEqual(outputFilePath, inputFilePath2))
			{

				string path = GetPath(outputFilePath);
				CreateMultipleDir(path);



				if (IsEqual(extention1, CNormalsDatabase::DATABASE_EXT))
				{
					msg = ExecuteNormal(inputFilePath1, inputFilePath2, outputFilePath, callback);
				}
				else if (IsEqual(extention1, CDailyDatabase::DATABASE_EXT))
				{
					msg = ExecuteDaily(inputFilePath1, inputFilePath2, outputFilePath, callback);
				}
				else if (IsEqual(extention1, CHourlyDatabase::DATABASE_EXT))
				{
					msg = ExecuteHourly(inputFilePath1, inputFilePath2, outputFilePath, callback);
				}
				else
				{
					msg.ajoute("Unknown database extension");
				}
			}
			else
			{
				msg.ajoute(GetString(IDS_BSC_SAME_NAMES));
			}
		}
		else
		{
			msg.ajoute("Database must be of the same type");
		}


		return msg;
	}
	//************************************************************************
	//normal

	ERMsg CMergeWeather::ExecuteNormal(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback)
	{
		ERMsg msg;

	//	TRY
	//{
		msg = CNormalsDatabase::DeleteDatabase(outputFilePath, callback);

		if (msg)
		{
			callback.AddMessage(GetString(IDS_CREATE_DATABASE));
//			callback.PushLevel();

			callback.AddMessage(outputFilePath);
			callback.AddMessage("");

			//Get the data for each station
			CNormalsDatabase outputDB;

			msg = outputDB.Open(outputFilePath, CNormalsDatabase::modeEdit);
			if (msg)
			{
				double distance = as<double>(DISTANCE);
				double deltaElev = as<double>(DELTA_ELEV);
				size_t mergeType = as<size_t>(MERGE_TYPE);
				

				msg = outputDB.CreateFromMerge(inputFilePath1, inputFilePath2, distance, deltaElev, mergeType, callback);
				outputDB.Close();

				if (msg)
				{
					msg = outputDB.Open(outputFilePath);
					outputDB.Close();
				}
			}

			
			//callback.PopLevel();
		}
		/*}
			CATCH_ALL(e)
		{
			msg = SYGetMessage(*e);
			msg.ajoute(GetString(IDS_INVALID_INPUT_TASK));
		}
		END_CATCH_ALL*/


			return msg;
	}


	//************************************************************************
	//Daily
	ERMsg CMergeWeather::ExecuteDaily(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		//load the WeatherUpdater
	/*	TRY
		{
			*/
		msg = CDailyDatabase::DeleteDatabase(outputFilePath, callback);


		if (msg)
		{
			//callback.SetNbStep(4);

			//Get the data for each station
			CDailyDatabase outputDB;

			msg += outputDB.Open(outputFilePath, CDailyDatabase::modeWrite, callback);
			if (msg)
			{
				double distance = as<double>(DISTANCE);
				double deltaElev = as<double>(DELTA_ELEV);
				size_t mergeType = as<size_t>(MERGE_TYPE);
				size_t priorityRule = as<size_t>(PRIORITY_RULE);
				string log;

				msg = outputDB.CreateFromMerge(inputFilePath1, inputFilePath2, distance, deltaElev, mergeType, priorityRule, log, callback);
				msg += outputDB.Close();
				if (msg)
				{
					msg = outputDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
					outputDB.Close();

					string filePath = outputFilePath + ".log.csv";


					ofStream file;
					msg += file.open(filePath);
					if (msg)
					{
						file.write(log.c_str(), log.length());
						file.close();
					}
				}
			}
		}
		//}
		//CATCH_ALL(e)
		//{
		//	msg = SYGetMessage(*e);
		//	//mettre 
		//	msg.asgType(ERMsg::ERREUR);
		//	msg.ajoute(GetString(IDS_INVALID_INPUT_TASK));
		//}
		//END_CATCH_ALL


			return msg;
	}


	//************************************************************************
	//Hourly
	ERMsg CMergeWeather::ExecuteHourly(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		//load the WeatherUpdater
	//	TRY

		msg = CHourlyDatabase().DeleteDatabase(outputFilePath, callback);

		if (msg)
		{
			//callback.SetNbStep(4);

			//Get the data for each station
			CHourlyDatabase outputDB;

			msg += outputDB.Open(outputFilePath, CHourlyDatabase::modeWrite, callback);
			if (msg)
			{
				double distance = as<double>(DISTANCE);
				double deltaElev = as<double>(DELTA_ELEV);
				size_t mergeType = as<size_t>(MERGE_TYPE);
				size_t priorityRule = as<size_t>(PRIORITY_RULE);

				string log;
				msg = outputDB.CreateFromMerge(inputFilePath1, inputFilePath2, distance, deltaElev, mergeType, priorityRule, log, callback);
				msg += outputDB.Close();
				if (msg)
				{
					msg = outputDB.Open(outputFilePath, CHourlyDatabase::modeRead, callback);
					outputDB.Close();

					string filePath = outputFilePath + ".log.csv";
					//SetFileExtension(filePath, "_log.csv");

					ofStream file;
					msg += file.open(filePath);
					if (msg)
					{
						file.write(log.c_str(), log.length());
						file.close();
					}
				}
			}
		}

	/*	}
			CATCH_ALL(e)
		{
			msg = SYGetMessage(*e);
			msg.ajoute(GetString(IDS_INVALID_INPUT_TASK));
		}
		END_CATCH_ALL
*/

			return msg;
	}


}