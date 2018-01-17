#pragma once

#include "GSODStationOptimisation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class CUIManitoba : public CTaskBase
	{
	public:

		//AGRI_HISTORICAL,
		enum TNetwork{AGRI, FIRE, HYDRO, NB_NETWORKS};
		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, NETWORK, DATA_TYPE, NB_ATTRIBUTES };
		static size_t GetNetwork(const std::string& network);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIManitoba); }

		CUIManitoba(void);
		virtual ~CUIManitoba(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

		static double GetWindDir(std::string compass);
		
		std::bitset<CUIManitoba::NB_NETWORKS> CUIManitoba::GetNetWork()const;
	protected:
		

		ERMsg UpdateStationsFile(CCallback& callback);

		

		std::string GetStationsListFilePath(size_t network)const;
		ERMsg GetAgriFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(size_t network, size_t type, const std::string& stationName, int year, size_t m = NOT_INIT)const;
		
		//ERMsg ReadAgriData(const std::string& filePath, CYear& dailyData)const;
		//ERMsg ExecuteAgriculture(CCallback& callback);
		ERMsg SplitAgriStations(const std::string& outputFilePath, CCallback& callback);
		//ERMsg ReadAgriData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const;

		ERMsg ExecuteHistoricalAgriculture(CCallback& callback);
		ERMsg GetHistoricalStationList(size_t dataType, StringVector& fileList, CCallback& callback);
		ERMsg DownloadStationData(UtilWWW::CHttpConnectionPtr& pConnection, size_t type, const std::string& ID, CTRef TRef, std::string& text);
		ERMsg SaveAgricultureDailyStation(const std::string& filePath, std::string str);
		ERMsg SaveAgricultureHourlyStation(const std::string& filePath, std::string str);
		
		ERMsg ExecuteFire(CCallback& callback);
		ERMsg SplitFireData(const std::string& outputFilePath, CCallback& callback);
		ERMsg UpdateFireStationsList(CLocationVector& locations, CCallback& callback);

		ERMsg ExecuteHydro(CCallback& callback);
		ERMsg UpdateHydroStationsList(CCallback& callback);
		ERMsg SplitHydroData(const std::string& ID, const StringVector& outputFilePath, CCallback& callback);

		CLocationVector m_stations;

		

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* SERVER_PATH[NB_NETWORKS];
		
		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* SUBDIR_NAME[NB_NETWORKS];

	};

}