#include "StdAfx.h"
#include "CreateGribsDB.h"
#include "UIRapidUpdateCycle.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "boost\dynamic_bitset.hpp"
#include "WeatherBasedSimulationString.h"
#include "Geomatic/SfcGribsDatabase.h"



using namespace std; 

namespace WBSF
{
	//*********************************************************************

	const char* CCreateGribsDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Type", "Input1", "Input2", "Input3", "Forecast1", "Forecast2", "Forecast3", "OutputFilePath", "Begin", "End" };
	const size_t CCreateGribsDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_COMBO_INDEX, T_UPDATER, T_UPDATER, T_UPDATER, T_UPDATER, T_UPDATER, T_UPDATER, T_FILEPATH, T_DATE, T_DATE };
	const UINT CCreateGribsDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_GRIBS_P;
	const UINT CCreateGribsDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_GRIBS;

	const char* CCreateGribsDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateGribs";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateGribsDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateGribsDB::CLASS_NAME(), (createF)CCreateGribsDB::create);


	CCreateGribsDB::CCreateGribsDB(void)
	{}


	CCreateGribsDB::~CCreateGribsDB(void)
	{}


	std::string CCreateGribsDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case MERGE_TYPE:str = "Take first available|Take all available"; break;
		case INPUT1:	str = GetUpdaterList(CUpdaterTypeMask(true, false, false, false, true)); break;
		case INPUT2:	str = GetUpdaterList(CUpdaterTypeMask(true, false, false, false, true)); break;
		case INPUT3:	str = GetUpdaterList(CUpdaterTypeMask(true, false, false, false, true)); break;
		case FORECAST1:	str = GetUpdaterList(CUpdaterTypeMask(true, false, true, false, true)); break;
		case FORECAST2:	str = GetUpdaterList(CUpdaterTypeMask(true, false, true, false, true)); break;
		case FORECAST3:	str = GetUpdaterList(CUpdaterTypeMask(true, false, true, false, true)); break;
		case OUTPUT:	str = GetString(IDS_STR_FILTER_GRIBS); break;
		};

		return str;
	}

	std::string CCreateGribsDB::Default(size_t i)const
	{
		string str;


		switch (i)
		{
		case MERGE_TYPE:	str = "0"; break;
		case FIRST_DATE:	
		case LAST_DATE:		str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break; //str = CTRef::GetCurrentTRef(CTM::HOURLY).GetFormatedString(); break;
		};

		return str;
	}

	CTPeriod CCreateGribsDB::GetPeriod()const
	{
		CTPeriod p;
		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));

		return p;
	}


	ERMsg CCreateGribsDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator
		
		ERMsg msg;


		size_t mergeType = as<size_t>(MERGE_TYPE);
		string outputFilePath = Get(OUTPUT);
		if (outputFilePath.empty())
		{
			msg.ajoute("Invalid output gribs file path");
			return msg;
		}

		SetFileExtension(outputFilePath, ".Gribs");
		string basePath = GetPath(outputFilePath);

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		
		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		CTPeriod p = GetPeriod();
		if (!p.IsInit())
		{
			msg.ajoute("Invalid period");
			msg.ajoute(p.GetFormatedString()); 
			return msg;
		}
	
		size_t nbGrib = 0;
		CGribsMap gribs;

		msg = RemoveFile(outputFilePath);

		size_t nbTask = 0;
		for (int i = INPUT1; i < 6 && msg; i++)
			if (!Get(i).empty())
				nbTask++;

		callback.PushTask("Gather gribs list (" +ToString(nbTask) + " sources)", nbTask);
	
	//get all 
		for (int i = 0; i < 6 && msg; i++)
		{
			if (!Get(INPUT1 + i).empty())
			{
				//load the WeatherUpdater
				CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT1 + i));

				if (pTask.get() != NULL)
				{
					ASSERT(pTask->IsGribs());
					CGribsMap gribsList;
					msg = pTask->GetGribsList(p, gribsList, callback);
					for (CGribsMap::const_iterator it = gribsList.begin(); it != gribsList.end() && msg; it++)
					{
						bool already = gribs.find(it->first) != gribs.end();
						if (!already || mergeType == ALL_AVAIL)
						{
							gribs[it->first].insert(gribs[it->first].begin(), it->second.begin(), it->second.end());
							nbGrib++;
						}
						msg += callback.StepIt(0);
					}
				}
				else
				{
					msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT1 + i)));
				}

				msg += callback.StepIt();
			}
		}

		callback.PopTask();


		if (msg)
		{
			
			msg = gribs.save(outputFilePath);

			p.End() = min(p.End(), now);
			
			size_t nbMissing = 0;
			for (CTRef TRef = p.Begin(); TRef < p.End() && msg; TRef++)
			{
				bool alvailable = gribs.find(TRef) != gribs.end();
				if (!alvailable)
					nbMissing++;

				msg += callback.StepIt(0);
			}

			callback.AddMessage("Nb gribs added: " + ToString(nbGrib) + " (" + to_string(Round(nbGrib / 24, 1)) + " days)");
			if(nbMissing>0)
				callback.AddMessage("WARNING: there are " + to_string(nbMissing) + " (" + to_string(Round(nbMissing / 24, 1)) + " days)" + " missing image for the period");

		}
		
		
	
				
		

		return msg;
	}


}