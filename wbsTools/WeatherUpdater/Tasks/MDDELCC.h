#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{


	//**************************************************************
	class CMDDELCC 
	{
	public:
		CMDDELCC(void);
		virtual ~CMDDELCC(void);

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		std::string GetStationListFilePath()const;


		std::string m_workingDir;
		int m_firstYear;
		int m_lastYear;
		bool bForceUpdateList;
		int m_updateUntil;

	protected:

		
		std::string GetOutputFilePath(int year, size_t m, const std::string& stationName, bool bProhibited=false)const;


		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback)const;
		ERMsg DownloadStation(UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback);
		ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& ID, int year, size_t m, const std::string& filePath);
		

		ERMsg ReadData(const std::string& filePath, CWeatherYear& dailyData)const;
		
		CLocationVector m_stations;

		static const char* SERVER_NAME;
	};



}

