#pragma once


#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
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
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		//virtual void UpdateLanguage();

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
		void GetStationInformation(__int64, CLocation& station)const;


		//Update station list part


		int GetNbStation(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page)const;

		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback);
		ERMsg GetStationListPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page, CLocationVector& stationList)const;
		ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
		static ERMsg UpdateStationList(CLocationVector& stationList, CEnvCanStationMap& stationMap, CCallback& callback);
		static ERMsg UpdateCoordinate(UtilWWW::CHttpConnectionPtr& pConnection, __int64 id, int year, size_t m, CLocation& station);



		//Update data part
		ERMsg ParseStationDataPage(const std::string& sourceIn, CLocation& station, std::string& parsedText)const;

		bool NeedDownload(const std::string& filePath, const CLocation& info, int y)const;
		ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, __int64 id, int year, const std::string& page);
		//std::string GetForecastListFilePath()const{ return GetApplicationPath() + "ForecastLinkEnvCan.csv"; }
		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "DailyStationsList.xml"; }
		ERMsg DownloadStation(UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback);
		ERMsg CleanStationList(CLocationVector& stationList, CCallback& callback)const;


		//CProvinceSelection m_selection;
		//CGeoRect m_boundingBox;
		//short m_bForceDownload;
		//bool m_bExtractSnow;

		//stat
		//void InitStat();
		//void AddToStat(short year);
		//void ReportStat(CCallback& callback);

		CEnvCanStationMap m_stations;
		//int m_nbDownload;
		//std::vector<int> m_stat;



		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, size_t m, size_t d);
		static CTPeriod String2Period(std::string period);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const char* SERVER_NAME;
	};

}

