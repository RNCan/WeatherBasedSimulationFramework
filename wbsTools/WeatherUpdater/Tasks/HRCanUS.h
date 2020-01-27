#pragma once



#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "TaskBase.h"


namespace WBSF
{

	class CHRCanUS
	{
	public:

		CHRCanUS(const std::string& workingDirCan, const std::string& workingDirUS, const std::string& workingDir);
		virtual ~CHRCanUS(void);

		size_t m_update_last_n_days;
		
		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg CreateHourlyCanUS(std::set<std::string> outputPath, CCallback& callback);
		ERMsg CreateDailyCanUS(std::set<std::string> outputPath, CCallback& callback);
		ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback);


	protected:

		std::string m_workingDir;
		std::string m_workingDirCan;
		std::string m_workingDirUS;
	};

}