#pragma once

#include "ProvinceSelection.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{

	//**************************************************************
	class CUIEnvCanHourly : public CTaskBase
	{
	public:
		
		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, PROVINCE, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanHourly); }

		CUIEnvCanHourly(void);
		virtual ~CUIEnvCanHourly(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual bool IsHourly()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual const std::string& Title(size_t i)const{ ASSERT(i<NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Default(size_t i)const;
		virtual std::string Option(size_t i)const;

	protected:

		std::string GetOutputFilePath(const std::string& prov, int year, size_t m, const std::string& stationName)const;
		void GetStationInformation(__int64 ID, CLocation& station)const;


		//Update station list part
		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg CleanStationList(CLocationVector& stationList, CCallback& callback)const;
		int GetNbStation(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page)const;
		ERMsg UpdateCoordinate(UtilWWW::CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, size_t d, CLocation& station)const;
		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "HourlyStationsList.xml"; }

		ERMsg GetStationListPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page, CLocationVector& stationList)const;
		ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
		ERMsg UpdateStationList(CLocationVector& stationList, CEnvCanStationMap& stationMap, CCallback& callback)const;


		//Update data part
		ERMsg DownloadStation(UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback);
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback = DEFAULT_CALLBACK)const;

	

		bool NeedDownload(const std::string& filePath, const CLocation& info, int year, size_t m)const;
		ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, const std::string& page);



		//CProvinceSelection m_selection;
		//CGeoRect m_boundingBox;
		//size_t m_firstMonth;
		//size_t m_lastMonth;
		//bool m_bForceDownload;

		//bool m_bExtractWindDir;
		//bool m_bExtractVaporPressure;
		//bool m_bExtractPressure;

		//long m_nbDays;

		//stat
		//void InitStat();
		//void AddToStat(int year);
		//void ReportStat(CCallback& callback);

		CEnvCanStationMap m_stations;
		//int m_nbDownload;
		//std::vector<int> m_stat;

		static CTPeriod String2Period(std::string period);
		static void UpdatePeriod(CLocation& location, int year);
		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, size_t m, size_t d);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const StringVector ATTRIBUTE_TITLE;
		static const char* SERVER_NAME;

	};

}