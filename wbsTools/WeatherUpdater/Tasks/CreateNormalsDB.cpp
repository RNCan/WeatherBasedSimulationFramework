#include "StdAfx.h"
#include "CreateNormalsDB.h"
#include "TaskFactory.h"
#include "Basic/NormalsStation.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDatabase.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"
#include "Simulation/AdvancedNormalStation.h"


#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std; 

namespace WBSF
{
	//********************************************************************* 

	const char* CCreateNormalsDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "InputFilePath", "OutputFilePath", "FirstYear", "LastYear", "NbYears", "ApplyClimaticChange", "MMGFilepath", "ReferencePeriod", "FuturPeriod", "NbNeighbor", "MaxDistance", "Power" };
	const size_t CCreateNormalsDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_STRING, T_STRING, T_STRING, T_BOOL, T_FILEPATH, T_COMBO_STRING, T_STRING_SELECT, T_STRING, T_STRING, T_STRING };
	const UINT CCreateNormalsDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_NORMALS_P;
	const UINT CCreateNormalsDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_NORMALS;

	const char* CCreateNormalsDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateNormals";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateNormalsDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateNormalsDB::CLASS_NAME(), (createF)CCreateNormalsDB::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("NormalDBFromDailyDB", (createF)CCreateNormalsDB::create);


	CCreateNormalsDB::CCreateNormalsDB(void)
	{}


	CCreateNormalsDB::~CCreateNormalsDB(void)
	{}


	std::string CCreateNormalsDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:			str = GetString(IDS_STR_FILTER_DAILY); break;
		case OUTPUT:		str = GetString(IDS_STR_FILTER_NORMALS); break;
		case MMG_FILEPATH:	str = GetString(IDS_STR_FILTER_MMG); break;
		case NORMAL_PERIOD: 
		{
			//str = " ";
			for (int i = 0; i < 4; i++)
			{
				if (!str.empty())
					str += "|";

				str += FormatA("%d-%d", 1961 + 10 * i, 1990 + 10 * i);
			}
				

			//str += FormatA("|2003-2017");//special case for Quebec database
			break;
		}
		case FUTUR_PERIOD:
		{
			for (size_t i = 0; i < CNormalFromDaily::NB_CC_PERIODS; i++)
			{
				str += i > 0 ? "|" : "";
				str += FormatA("%d=%d-%d", i, CNormalFromDaily::FIRST_YEAR_OF_FIRST_PERIOD + 10 * i, (CNormalFromDaily::FIRST_YEAR_OF_FIRST_PERIOD+29) + 10 * i);
			}
				

			break;
		}
		};

		return str;
	}

	std::string CCreateNormalsDB::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:	str = "1991"; break;
		case LAST_YEAR:		str = "2020"; break;
		case NB_YEARS_MIN:	str = "10"; break;
		case APPLY_CLIMATIC_CHANGE:	str = "0"; break;
		case NORMAL_PERIOD:	str = "1991-2020"; break;
		case FUTUR_PERIOD:	str = "3|4|5|6|7|8|9|10|11"; break;
		case NB_NEIGHBOR:	str = "3"; break;
		case MAX_DISTANCE:	str = "500"; break;
		case POWER:			str = "2"; break;
		};

		return str;
	}

	ERMsg CCreateNormalsDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator
		
		ERMsg msg;

		if (as<int>(NB_YEARS_MIN)<= 1)
		{
			msg.ajoute("The number of year must be greater then 1");
			return msg;
		}

		string outputFilePath = Get(OUTPUT);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		SetFileExtension(outputFilePath, CNormalsDatabase::DATABASE_EXT);

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		msg = CNormalsDatabase::DeleteDatabase(outputFilePath, callback);

		if (msg)
		{ 
			CNormalFromDaily normalFromDaily;
			normalFromDaily.m_firstYear = as<int>(FIRST_YEAR);
			normalFromDaily.m_lastYear = as<int>(LAST_YEAR);
			normalFromDaily.m_nbYearMin = as<int>(NB_YEARS_MIN);
			normalFromDaily.m_bApplyCC = as<bool>(APPLY_CLIMATIC_CHANGE);
			normalFromDaily.m_inputMMGFilePath = Get(MMG_FILEPATH);
			//normalFromDaily.m_bCreateAll = false;
			StringVector ref ( Get(NORMAL_PERIOD), "-");
			if (ref.size() == 2)
			{
				//normalFromDaily.m_firstYear = .m_refPeriodIndex = as<int>(NORMAL_PERIOD)-1;
				normalFromDaily.m_firstRefYear = WBSF::as<int>(ref[0]);
				normalFromDaily.m_nbRefYears = WBSF::as<int>(ref[1]) - WBSF::as<int>(ref[0]) + 1;
				normalFromDaily.m_CCPeriodIndex = GetCCPeriod();
				normalFromDaily.m_inputDBFilePath = Get(INPUT);
				normalFromDaily.m_outputDBFilePath = outputFilePath;
				normalFromDaily.m_nbNeighbor = as<double>(NB_NEIGHBOR);
				normalFromDaily.m_maxDistance = as<double>(MAX_DISTANCE) * 1000;//[km] to [m]
				normalFromDaily.m_power = as<double>(POWER);
			}

			msg = normalFromDaily.CreateNormalDatabase(callback);
			
		}

		

		return msg;
	}

	CNormalFromDaily::CCPeriodBitset CCreateNormalsDB::GetCCPeriod()const
	{
		CNormalFromDaily::CCPeriodBitset CCPeriod;
		StringVector index(Get(FUTUR_PERIOD), "|");
		for (size_t i = 0; i < index.size(); i++)
			if (ToInt(index[i]) < CCPeriod.size())
				CCPeriod.set(ToInt(index[i]));

		return CCPeriod;
	}
	

}