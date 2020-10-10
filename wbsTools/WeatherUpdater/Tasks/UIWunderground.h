#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/UtilWWW.h"


namespace WBSF
{

	//**************************************************************
	class CUIWunderground : public CTaskBase
	{
	public:
		
		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum Tattributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, DATA_TYPE, /*COUNTRIES, STATES, PROVINCE,*/ STATION_LIST, UPDATE_MISSING,/*GOLD_STAR,*/ NB_ATTRIBUTES };//UPDATE_STATION_LIST, 

		//static size_t GetNetwork(const std::string& network_name);
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIWunderground); }

		CUIWunderground(void);
		virtual ~CUIWunderground(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const { return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }

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
		std::string GetOutputFilePath(const std::string& type, const std::string& country, const std::string& states, const std::string& ID, int year);
		ERMsg LoadStationList(const std::string& apiKey, CLocationVector& stationList, CCallback& callback)const;
		//ERMsg ExtractElevation(CLocationVector& stationList, CCallback& callback);
		//ERMsg DownloadStationList(const std::string& country, CLocationVector& stationList, CCallback& callback)const;
		//ERMsg CompleteStationList(CLocationVector& stationList2, CCallback& callback)const;
		//ERMsg LoadStationList(CCallback& callback);
		//ERMsg UpdateStationList(UtilWWW::CHttpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadDailyData(const std::string& filePath, CWeatherStation& data, CCallback& callback)const;
		ERMsg ReadHourlyData(const std::string& file_path, CWeatherStation& data, CCallback& callback)const;
		
		//ERMsg SaveData(size_t type, const std::string& filePath, const std::string& str, CCallback& callback);
		ERMsg ParseJson(size_t type, const std::string& str, CWeatherYears& data, CCallback& callback)const;
		ERMsg ParseJsonMetadata(const std::string& str, CLocation& location, CCallback& callback)const;
		

		CLocationVector m_stations;

		//static HOURLY_DATA::TVarH GetVar(const std::string& str);
		//static CWVariables GetVariables(std::string str);
		//static void CleanString(std::string& str);
		//static double GetCoordinate(std::string str);
		static ERMsg ExtractNominatimName(CLocation& locations, CCallback& callback);
		static bool NeedDownload(int year, const std::string& ouputFilePath);
		//static CTPeriod GetHourlyPeriod(const std::string& filePath, double& last_hms);
		//static void clean_source(double last_hms, std::string& source);

		static const char* get_type_name(size_t type);
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}