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
	class CEnvCanGribForecast
	{
	public:

		//forecast type
		enum{ GT_HRDPS, GT_RDPS };
		enum TVariables{ PRES_SFC_0, TMP_TGL_2, DPT_SFC_0, APCP_SFC_0, WIND_TGL_10, WDIR_TGL_10, WEARN_SFC_0, NB_FORECAST_VAR };
		static const char* FORECAST_VAR_NAME[NB_FORECAST_VAR];
		static const HOURLY_DATA::TVarH FORECAST_VARIABLES[NB_FORECAST_VAR];

		size_t m_type;
		std::string m_workingDir;

		CEnvCanGribForecast(void);
		virtual ~CEnvCanGribForecast(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

	protected:

		ERMsg DownloadGrib(CCallback& callback);

		std::string GetOutputFilePath(const std::string& filetitle)const;
		std::string GetRemoteFilePath(size_t HH, size_t hhh, const std::string& filetitle)const;
		

		ERMsg RemoveOldForecast(CCallback& callback);
		
		size_t GetHH(const std::string& title)const;
		size_t Gethhh(const std::string& title)const;
		size_t GetLatestHH(UtilWWW::CHttpConnectionPtr& pConnection)const;
		CTRef GetTRef(const std::string& title)const;
		size_t GetVariable(const std::string& fileTitle)const;
		ERMsg OpenDatasets(CCallback& callback);

		std::array<std::array<CGDALDatasetEx, NB_FORECAST_VAR>, 52> m_datasets;
		CProjectionTransformation m_geo2gribs;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}