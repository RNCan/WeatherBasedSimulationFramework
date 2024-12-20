#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class CUINewBrunswick : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TNetwork { FIRE_HISTORICAL, AGRI_DAILY, AGRI_HOURLY, FIRE_HOURLY, NB_NETWORKS };
		enum TAttributes { WORKING_DIR, NETWORK, FIRST_YEAR, LAST_YEAR, USER_NAME, PASSWORD, SHOW_CURL, NB_ATTRIBUTES };
		static size_t GetNetworkFromName(std::string name);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUINewBrunswick); }

		CUINewBrunswick(void);
		virtual ~CUINewBrunswick(void);


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
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:
		
		ERMsg ExecuteHistoricalFire(CCallback& callback);
		//ERMsg ExecutePublicFire(CCallback& callback);
		ERMsg ExecuteAgricultureDaily(CCallback& callback);
		ERMsg ExecuteAgricultureHourly(CCallback& callback);
		

		ERMsg ExecuteFireHourly(CCallback& callback);
		ERMsg SplitFireHourly(const std::string& source, CCallback& callback);


		
		ERMsg SplitAgriStation(const std::string& outputFilePath);
		ERMsg DownloadAgriStationDaily(const std::string& filePath, int year);

		ERMsg ReadFireHistorical(const std::string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const;
		ERMsg DownloadAgriStationHourly(const std::string& ID, int year, std::string& str);
		ERMsg SaveAgriStationHourly(const std::string& filePath, std::string str);
		

		std::bitset<NB_NETWORKS> GetNetWork()const;

		std::string GetStationListFilePath()const;

		ERMsg GetFileList(size_t n, CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg GetFileList(size_t n, StringVector& fileList, CCallback& callback)const;
		std::string GetOutputFilePath(size_t n, const std::string& stationName, int year)const;
		//ERMsg UpdateOldFile(CCallback& callback);
		
		CLocationVector m_stations;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* NETWORK_NAME[NB_NETWORKS];

	};

}