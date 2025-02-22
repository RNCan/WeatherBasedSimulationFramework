#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{
	class CShapeFileBase;

	class CUIACIS : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TAttributes { WORKING_DIR, DATA_TYPE, FIRST_YEAR, LAST_YEAR, UPDATE_STATIONS_LIST, IGNORE_ENV_CAN, MONTH_LAG, USER_NAME, PASSWORD, NB_ATTRIBUTES };
		//USER_NAME, PASSWORD, 
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIACIS); }

		CUIACIS(void);
		virtual ~CUIACIS(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDatabase()const{ return true; }
		virtual bool IsHourly()const{ return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
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

		ERMsg DownloadStationList(CCallback& callback = DEFAULT_CALLBACK)const;
		double GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII)const;

		ERMsg DownloadStation(CCallback& callback);
		ERMsg DownloadMonth(UtilWWW::CHttpConnectionPtr& pConnection, int year, size_t m, const std::string& ID, const std::string& filePath, CCallback& callback);
		ERMsg VerifyUserPass(CCallback& callback);
		//ERMsg ReadDataFile(const std::string& filePath, CWeatherStation& station);
		//ERMsg ReadData(std::stringstream& stream, CWeatherYears data);

		static bool IsInclude(const CLocation& station);
		bool NeedUpdate(const CLocation& station, int year, size_t m = LAST_MONTH);
		ERMsg DownloadStationListII(CCallback& callback)const;
		std::string GetSessiosnID(UtilWWW::CHttpConnectionPtr& pConnection);
		
		ERMsg DownloadStationHourly(CCallback& callback);
		ERMsg DownloadStationDaily(CCallback& callback);
		ERMsg SaveDataCSV(size_t type, const std::string& filePath, const std::string& str, CCallback& callback);


		std::string GetStationListFilePath()const;
		std::string GetOutputFilePath(int year, size_t m, const std::string& stationID)const;

		CLocationVector m_stations;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}