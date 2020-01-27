#include "StdAfx.h"
#include "UIHRDPA.h"
#include "HRDPA.h"
#include "Basic/FileStamp.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "../Resource.h"
#include "TaskFactory.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{



	//http://climate.weather.gc.ca/radar/image.php?time=04-MAR-14%2005.21.06.293480%20PM&site=WBI
	////HRDPA:
	//http://dd.weather.gc.ca/analysis/precip/hrdpa/grib2/polar_stereographic/24/


	//*********************************************************************
	const char* CUIHRDPA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product", "Type", "MaxHour" };
	const size_t CUIHRDPA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING };
	const UINT CUIHRDPA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_PRCP_RADAR_P;
	const UINT CUIHRDPA::DESCRIPTION_TITLE_ID = ID_TASK_EC_PRCP_RADAR;

	const char* CUIHRDPA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HRDPA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHRDPA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHRDPA::CLASS_NAME(), (createF)CUIHRDPA::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("EnvCanRadarPrcp", (createF)CUIHRDPA::create);

	CUIHRDPA::CUIHRDPA(void)
	{}

	CUIHRDPA::~CUIHRDPA(void)
	{}
	
	std::string CUIHRDPA::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT:	str = "RDPA|HRDPA"; break;
		case TYPE:	str = "06|24"; break;
		};
		return str;
	}

	std::string CUIHRDPA::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\HRDPA\\"; break;
		case PRODUCT: str = "1"; break;
		case TYPE: str = ToString(TYPE_06HOURS); break;
		case MAX_HOUR: str = "48"; break;
		};

		return str;
	}


	ERMsg CUIHRDPA::Execute(CCallback& callback)
	{
		CHRDPA HRDPA;
		HRDPA.m_bByHRDPS = false;
		HRDPA.m_workingDir = GetDir(WORKING_DIR);
		HRDPA.m_type = (CHRDPA::TPrcpPeriod)as<size_t>(TYPE);
		HRDPA.m_product = (CHRDPA::TProduct)as<size_t>(PRODUCT);
		HRDPA.m_max_hours = as<int>(MAX_HOUR);

		return HRDPA.Execute(callback);
	}



}