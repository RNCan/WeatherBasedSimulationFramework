#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	class CUIACISDaily : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIACISDaily); }

		CUIACISDaily(void);
		virtual ~CUIACISDaily(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		ERMsg GetStationList(CLocationVector& stationList, CCallback& callback)const;
		ERMsg DownloadStationDaily(CCallback& callback);
		std::string GetOutputFilePath(int year, const std::string& stationID)const;
		std::string GetOutputFilePath(int year, size_t m, const std::string& stationID)const;
		std::string GetForecastFilePath(const std::string& stationID)const;

		bool IsFileInclude(const std::string& fileTitle)const;
		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CYear& dailyData, CCallback& callback)const;
		void GetStationInformation(const std::string& fileTitle, CLocation& station)const;


		//Update data part
		ERMsg ParseStationDataPage(const std::string& sourceIn, CWeatherStation& station, std::string& parsedText)const;
		bool IsValid(const CLocation& info, short y, short m, const std::string& filePath)const;
		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, int m, int d);

		std::string GetStationListFilePath()const{ return (std::string)GetDir(WORKING_DIR) + "StationsList.csv"; }

		CLocationVector m_stations;

		static std::string GetSessiosnID(UtilWWW::CHttpConnectionPtr& pConnection);
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const char* SERVER_NAME;
	};

}