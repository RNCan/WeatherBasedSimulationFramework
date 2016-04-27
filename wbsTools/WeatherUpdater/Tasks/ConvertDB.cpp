#include "stdafx.h"
#include <boost/functional/factory.hpp>
#include <boost/bind.hpp>

#include "ConvertDB.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/UtilStd.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "WeatherBasedSimulationString.h"
#include "../resource.h"


using namespace std;
//using namespace WBSF::WEATHER;


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
		case DIRECTION:		str = GetString(IDS_CONVERT_DIRECTION); break;
		case INPUT_FILEPATH:str = GetString(IDS_STR_FILTER_WEATHER); break;
		case OUTPUT_FILEPATH:str = GetString(IDS_STR_FILTER_WEATHER); break;
		};
		return str;
	}

	std::string CConvertDB::Default(size_t i)const
	{
		std::string str;
		if (i == DIRECTION)
			str = ToString(TO_NEW_VERSION);

		return str;
	}


	ERMsg CConvertDB::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t direction = ToSizeT(Get(DIRECTION));
		string inputFilepath = GetAbsoluteFilePath(Get(INPUT_FILEPATH));
		string outputFilepath = GetAbsoluteFilePath(Get(OUTPUT_FILEPATH));
		string extention = GetFileExtension(inputFilepath);
		string extentionOut = GetFileExtension(extention);
		if (IsEqual(extention, ".NormalsStations") || IsEqual(extention, ".Normals"))
			extentionOut = (direction == TO_NEW_VERSION) ? ".NormalsStations" : ".Normals";

		SetFileExtension(outputFilepath, extentionOut);

		
		if (IsEqual(outputFilepath, inputFilepath))
		{
			msg.ajoute(GetString(IDS_BSC_SAME_NAMES));
			return msg;
		}

		CreateMultipleDir(GetPath(outputFilepath));

		if (IsEqualNoCase(extention, ".Normals") || IsEqualNoCase(extention, ".NormalsStations"))
		{
			if (direction == TO_NEW_VERSION)
			{
				msg = CNormalsDatabase::DeleteDatabase(outputFilepath, callback);
				if (msg)
					msg = CNormalsDatabase::v6_to_v7(inputFilepath, outputFilepath, callback);
			}
			else
			{
				msg = WBSF::RemoveFile(outputFilepath);
				if (msg)
					msg = CNormalsDatabase::v7_to_v6(inputFilepath, outputFilepath, callback);
			}

		}
		else if (IsEqualNoCase(extention, ".DailyStations"))
		{
			msg = CDailyDatabase::DeleteDatabase(outputFilepath, callback);
			if (msg)
			{
				if (direction == TO_NEW_VERSION)
					msg = CDailyDatabase::v3_to_v4(inputFilepath.c_str(), outputFilepath.c_str(), callback);
				else
					msg = CDailyDatabase::v4_to_v3(inputFilepath.c_str(), outputFilepath.c_str(), callback);
			}
		}
		else
		{
			msg.ajoute("Unknown database type. Verify file extension.");
		}

		return msg;
	}


}
