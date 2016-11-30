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
	class CHRDPS
	{
	public:

		CHRDPS(const std::string& workingDir);
		virtual ~CHRDPS(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetVirtuelStation(const CLocationVector& stations, CWVariables variables, CTPeriod p, CWeatherStation& station, CCallback& callback = DEFAULT_CALLBACK);

	protected:

		std::string m_workingDir;


		std::string GetOutputFilePath(const std::string& filetitle)const;
		std::string GetRemoteFilePath(size_t HH, size_t hhh, const std::string& filetitle)const;

		size_t GetHH(const std::string& title)const;
		size_t Gethhh(const std::string& title)const;
		size_t GetLatestHH(UtilWWW::CHttpConnectionPtr& pConnection)const;
		CTRef GetTRef(const std::string& title)const;
		size_t GetVariable(const std::string& fileTitle)const;
		ERMsg OpenDatasets(CCallback& callback);

		//std::array<std::array<CGDALDatasetEx, NB_FORECAST_VAR>, 52> m_datasets;
		//CProjectionTransformation m_geo2HRDPS;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}