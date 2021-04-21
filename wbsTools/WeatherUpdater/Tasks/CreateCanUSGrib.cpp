#include "StdAfx.h"
#include "CreateCanUSGrib.h"
#include "HRCanUS.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{
	//*********************************************************************
	const char* CCreateCanUSGrib::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "HRDPSPath", "HRRRPath", "OutputPath", "UpdateLastNDays"};
	const size_t CCreateCanUSGrib::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_PATH, T_PATH };
	const UINT CCreateCanUSGrib::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_CANUS_P;
	const UINT CCreateCanUSGrib::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_CANUS;


	const char* CCreateCanUSGrib::CLASS_NAME() { static const char* THE_CLASS_NAME = "CreateCanUSGrid";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateCanUSGrib::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateCanUSGrib::CLASS_NAME(), CCreateCanUSGrib::create);


	CCreateCanUSGrib::CCreateCanUSGrib(void)
	{}

	CCreateCanUSGrib::~CCreateCanUSGrib(void)
	{}

	std::string CCreateCanUSGrib::Option(size_t i)const
	{
		string str;

		//switch (i)
		//{
		//};
		
		return str;
	}

	std::string CCreateCanUSGrib::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case UPDATE_LAST_N_DAYS: str = "7"; break;
		};

		return str;
	}

	ERMsg CCreateCanUSGrib::Execute(CCallback& callback)
	{
		ERMsg msg;

		//GDALSetCacheMax64(128 * 1024 * 1024);

		/*string inputFilePath = Get(INPUT_FILEPATH);
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
*/

		CHRCanUS HRCanUS(GetDir(HRDPS_PATH), GetDir(HRRR_PATH), GetDir(OUTPUT_PATH));
		HRCanUS.m_update_last_n_days = as<size_t>(UPDATE_LAST_N_DAYS);

		if (msg)
			msg += HRCanUS.Execute(callback);

		if(msg)
			msg += HRCanUS.CreateCanUSGribList(callback);


		return msg;
	}




	ERMsg CCreateCanUSGrib::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		/*int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector list1;
			string filter = m_compute_prcp ? "\\*f00.tif" : "\\*f00.grib2";
			list1 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, true);

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetLocalTRef(list1[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = list1[i];
			}

		}
*/


		return msg;
	}
}