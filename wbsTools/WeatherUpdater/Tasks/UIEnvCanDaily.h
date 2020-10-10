#pragma once


#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
//#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"


namespace WBSF
{





	//**************************************************************
	class CUIEnvCanDaily : public CTaskBase
	{
	public:

		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, PROVINCE, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanDaily); }

		CUIEnvCanDaily(void);
		virtual ~CUIEnvCanDaily(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDatabase()const{ return true; }
		virtual bool IsDaily()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual const std::string& Title(size_t i)const{ ASSERT(i<NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Default(size_t i)const;
		virtual std::string Option(size_t i)const;

	protected:

		

		std::string GetOutputFilePath(const std::string& prov, int year, const std::string& stationName)const;

		ERMsg ReadData(const std::string& filePath, CWeatherYear& dailyData)const;
		void GetStationInformation(const std::string& ID, CLocation& station)const;


		//Update station list part


		ERMsg GetNbStation(const std::string& URL, size_t& nbStation)const;

		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback);
		ERMsg GetStationListPage(const std::string& URL, CLocationVector& stationList)const;
		ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
		//ERMsg UpdateAllStationList(CCallback& callback);
		static ERMsg UpdateStationList(CLocationVector& stationList, CLocationVector& stations, CCallback& callback);
		static ERMsg UpdateCoordinate(__int64 id, int year, size_t m, CLocation& station);



		//Update data part
		ERMsg ParseStationDataPage(const std::string& sourceIn, CLocation& station, std::string& parsedText)const;

		bool NeedDownload(const std::string& filePath, const CLocation& info, int y)const;
		//ERMsg CopyStationDataPage(__int64 id, int year, const std::string& page, CCallback& callback);
		//std::string GetForecastListFilePath()const{ return GetApplicationPath() + "ForecastLinkEnvCan.csv"; }
		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "DailyStationsList.csv"; }
		ERMsg DownloadStation(const CLocation& info, CCallback& callback);
		

		CLocationVector m_stations;

		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, size_t m, size_t d);
		static CTPeriod String2Period(std::string period);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}

