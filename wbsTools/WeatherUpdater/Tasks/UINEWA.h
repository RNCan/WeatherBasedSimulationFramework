#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	class CUINEWA : public CTaskBase
	{
	public:

		enum TNEWAStates { NB_NEWA_STATES=25};

//		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, STATES, UPDATE_UNTIL, UNPDATE_STATION_LIST, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUINEWA); }

		CUINEWA(void);
		virtual ~CUINEWA(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDatabase()const{ return true; }
		virtual bool IsHourly()const { return true; }//{ return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
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

		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg DownloadStation(CCallback& callback);
		//ERMsg DownloadStationHourly(CCallback& callback);
		ERMsg DownloadMonth(UtilWWW::CHttpConnectionPtr& pConnection, int year, size_t m, const std::string& ID, const std::string& filePath, CCallback& callback);
		



		std::string GetStationListFilePath()const;
		std::string GetOutputFilePath(const std::string& stationID, int year)const;
		ERMsg MergeData( const std::string& ID, std::string source, CCallback& callback);


		static ERMsg GetText(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& ID, int year, size_t m, std::string& text);
		static bool IsInclude(size_t state);
		static std::string GetStatesPossibleValue();




		CLocationVector m_stations;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME1;
		static const char* SERVER_NAME2;
	};

}