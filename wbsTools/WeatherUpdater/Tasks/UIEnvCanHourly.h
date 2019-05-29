#pragma once

#include "ProvinceSelection.h"
#include "Basic/WeatherStation.h"
//#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

//namespace cctz{ class time_zone; }


namespace WBSF
{

	
	//**************************************************************
	class CUIEnvCanHourly : public CTaskBase
	{
	public:
		
		enum TSWOBVar{ NB_SWOB_VARIABLES = 22 };
		enum TNetwork{ N_HISTORICAL, N_SWOB, NB_NETWORKS };
		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, PROVINCE, NETWORK, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanHourly); }

		CUIEnvCanHourly(void);
		virtual ~CUIEnvCanHourly(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }
		

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;
		virtual std::string Option(size_t i)const;

	protected:

		typedef std::array<std::string, NB_SWOB_VARIABLES * 2 + 4> SWOBDataHour;
		typedef std::array< std::array< SWOBDataHour, 24>, 31> SWOBData;


		ERMsg ExecuteHistorical(CCallback& callback);
		std::string GetOutputFilePath(size_t n, const std::string& prov, int year, size_t m, const std::string& stationName)const;
		std::bitset<CUIEnvCanHourly::NB_NETWORKS> GetStationInformation(const std::string& ID, CLocation& station)const;


		//Update station list part
		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg CleanStationList(CLocationVector& stationList, CCallback& callback)const;
		int GetNbStation(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page)const;
		ERMsg UpdateCoordinate(UtilWWW::CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, size_t d, CLocation& station)const;
		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "HourlyStationsList.csv"; }

		ERMsg GetStationListPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page, CLocationVector& stationList)const;
		ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
		ERMsg UpdateStationList(CLocationVector& stationList, CLocationVector & stations, CCallback& callback)const;
		std::bitset<CUIEnvCanHourly::NB_NETWORKS> GetNetWork()const;

		//Update data part
		ERMsg DownloadStation(UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback);
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback = DEFAULT_CALLBACK)const;

		ERMsg ExecuteSWOB(CCallback& callback);
		std::string GetSWOBStationsListFilePath()const;
		ERMsg UpdateSWOBLocations(CCallback& callback);
		ERMsg GetSWOBLocation(const std::string& filePath, CLocation& location);
		ERMsg GetSWOBList(const CLocationVector& locations, std::map<std::string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback);
		ERMsg UpdateMissingLocation(CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback);
		ERMsg DownloadSWOB(const CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, CCallback& callback);
		ERMsg ReadSWOBData(const std::string& filePath, CTM TM, CWeatherStation& data, CCallback& callback);
		ERMsg ParseSWOB(CTRef TRef, const std::string& source, SWOBDataHour& data, CCallback& callback);
		ERMsg UpdateLastUpdate(const std::map<std::string, CTRef>& lastUpdate);

		ERMsg ReadSWOB(const std::string& filePath, SWOBData& data);
		ERMsg SaveSWOB(const std::string& filePath, const SWOBData& data);
		
		//ERMsg ReadSWOB_XML(const std::string& source, CHourlyData& data, CCallback& callback);
		//ERMsg SaveSWOB(CTRef TRef, const std::string& source, const std::string& filePath, CCallback& callback);
		
		

		bool NeedDownload(const std::string& filePath, const CLocation& info, int year, size_t m)const;
		ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, const std::string& page, CCallback& callback);

		
		CLocationVector m_stations;
		CLocationVector m_SWOBstations;

		static CTPeriod String2Period(std::string period);
		//static void UpdatePeriod(CLocation& location, int year);
		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, size_t m, size_t d);
		static CTRef GetSWOBTRef(const std::string & fileName);
		static std::string GetProvinceFormID(const std::string& ID);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* SWOB_VARIABLE_NAME[NB_SWOB_VARIABLES];
		static const char* DEFAULT_UNIT[NB_SWOB_VARIABLES];
		static const HOURLY_DATA::TVarH VARIABLE_TYPE[NB_SWOB_VARIABLES];
	};

}