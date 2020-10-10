#include "StdAfx.h"
#include "UIQualityControl.h"
#include "QualityControl.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{
	//*********************************************************************
	const char* CUIQualityControl::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "NormalsPath", "refPath", "controlPath", "OutputPath"};
	const size_t CUIQualityControl::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_FILEPATH };
	const UINT CUIQualityControl::ATTRIBUTE_TITLE_ID = IDS_TOOL_QUALITY_CONTROL_P;
	const UINT CUIQualityControl::DESCRIPTION_TITLE_ID = ID_TASK_QUALITY_CONTROL;


	const char* CUIQualityControl::CLASS_NAME() { static const char* THE_CLASS_NAME = "QualityControl";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIQualityControl::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIQualityControl::CLASS_NAME(), CUIQualityControl::create);


	CUIQualityControl::CUIQualityControl(void)
	{}

	CUIQualityControl::~CUIQualityControl(void)
	{}

	std::string CUIQualityControl::Option(size_t i)const
	{
		string str;

		//switch (i)
		//{
		//};
		
		return str;
	}

	std::string CUIQualityControl::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case UPDATE_LAST_N_DAYS: str = "7"; break;
		};

		return str;
	}

	ERMsg CUIQualityControl::Execute(CCallback& callback)
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

		CQualityControl QC;// (GetDir(HRDPS_PATH), GetDir(HRRR_PATH), GetDir(OUTPUT_PATH));
		msg += QC.Execute(callback);


		return msg;
	}




	ERMsg CUIQualityControl::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
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