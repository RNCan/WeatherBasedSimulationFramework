#include "StdAfx.h"
#include "CreateMMG.h"


#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std; 

namespace WBSF
{
	//*********************************************************************

	const char* CCreateMMG::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Input", "OutputFilePath"};
	const size_t CCreateMMG::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_FILEPATH};
	const UINT CCreateMMG::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_MMG_P;
	const UINT CCreateMMG::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_MMG;

	const char* CCreateMMG::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateMMG";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateMMG::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateMMG::CLASS_NAME(), (createF)CCreateMMG::create);


	CCreateMMG::CCreateMMG(void)
	{}


	CCreateMMG::~CCreateMMG(void)
	{}


	std::string CCreateMMG::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:		str = GetUpdaterList(CUpdaterTypeMask(false,true,false,false,true)); break;
		case OUTPUT:	str = GetString(IDS_STR_FILTER_MMG); break;
		};

		return str;
	}

	std::string CCreateMMG::Default(size_t i)const
	{
		string str;


		return str;
	}

	CTPeriod CCreateMMG::GetPeriod()const
	{
		CTPeriod p;

		return p;
	}


	ERMsg CCreateMMG::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator
		
		ERMsg msg;

		//load the WeatherUpdater
		CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT));


		if (pTask.get() != NULL)
		{
			string filePath = Get(OUTPUT);
			msg = pTask->CreateMMG(filePath, callback);
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT)));
		}
		

		return msg;
	}


}