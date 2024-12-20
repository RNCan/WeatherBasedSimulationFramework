#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

//namespace cctz{ class time_zone; }


namespace WBSF
{


	//**************************************************************
	class CUICIPRA : public CTaskBase
	{
	public:

		enum TNetwork { S_ATLANTIQUE, S_ONTARIO, S_POMME, S_QUEBEC, NB_NETWORKS};
		static size_t GetNetworkIndex(const std::string& network_name);

		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, NETWORKS, FIRST_YEAR, LAST_YEAR, FORECAST, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUICIPRA); }

		CUICIPRA(void);
		virtual ~CUICIPRA(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDatabase()const{ return true; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const{ return true; }
		

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationsFilePath();
		std::string GetMissingFilePath();

		ERMsg LoadStations(CCallback& callback);
		ERMsg UpdateStationsFile(CCallback& callback);
		std::string GetOutputFilePath(const std::string& network, int year, const std::string& name);
		ERMsg ReadDataFile(const std::string& filePath, CTM TM, CWeatherStation& station, CCallback& callback)const;
		ERMsg ReadForecastDataFile(const std::string& filePath, CTM TM, CWeatherStation& station, CCallback& callback)const;

		CLocationVector m_stations;
		
		
		std::string GetStationName(const std::string& ID)const;
		static int GetStationIndex(const StringVector& allFilePath, const std::string& stationName);
		std::string GetOutputFilePath(std::string filePath)const;

		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* NETWORK_TIMEZONE_NAME[NB_NETWORKS];
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SUB_DIR;
	};

}