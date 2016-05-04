#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"


namespace WBSF
{

	//**************************************************************
	class CUIWunderground : public CTaskBase
	{
	public:

		enum Tattributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, PROVINCE, NB_ATTRIBUTES };//UPDATE_STATION_LIST, 
		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };

		static size_t GetNetwork(const std::string& network_name);
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIWunderground); }

		CUIWunderground(void);
		virtual ~CUIWunderground(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return false; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;



	protected:

		std::string GetStationListFilePath(const std::string& country)const;
		std::string GetOutputFilePath(const std::string& country, const std::string& states, const std::string& ID, int year);
		ERMsg LoadStationList(CLocationVector& stationList, CCallback& callback)const;
		ERMsg ExtractElevation(CLocationVector& stationList, CCallback& callback);
		ERMsg DownloadStationList(const std::string& country, CLocationVector& stationList, CCallback& callback)const;
		ERMsg LoadStationList(CCallback& callback);
		ERMsg UpdateStationList(UtilWWW::CHttpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherStation& data, CCallback& callback)const;

		CLocationVector m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}