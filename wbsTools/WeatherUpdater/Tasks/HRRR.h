#pragma once



#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{


	//**************************************************************
	class CHRRR
	{
	public:
		
		CHRRR(const std::string& workingDir);
		virtual ~CHRRR(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);


		

	protected:

		std::string m_workingDir;
		std::string GetOutputFilePath(const std::string& filePath)const;

		CTRef GetTRef(const std::string& filePath)const;
		//ERMsg OpenDatasets(CCallback& callback);

//		std::array<std::array<CGDALDatasetEx, NB_FORECAST_VAR>, 52> m_datasets;
	//	CProjectionTransformation m_geo2gribs;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}