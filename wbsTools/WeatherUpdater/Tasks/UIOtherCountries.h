#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class CUIOtherCountries : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		//enum TCountries { IRELAND, NB_COUNTRIES };
		
		enum TNetwork { IRELAND_HISTORICAL, IRELAND_CURRENT, NB_NETWORKS };
		enum TAttributes { WORKING_DIR, NETWORKS, IE_COUNTIES, FIRST_YEAR, LAST_YEAR, DATA_TYPE, NB_ATTRIBUTES };
		static size_t GetNetworkFromName(std::string name);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIOtherCountries); }

		CUIOtherCountries(void);
		virtual ~CUIOtherCountries(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const { return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
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

		ERMsg ExecuteIreland(size_t n, size_t type, CCallback& callback);
		ERMsg DownloadIrelandStation(UtilWWW::CHttpConnectionPtr& pConnection, size_t n, size_t type, const std::string& ID, CCallback& callback);
		ERMsg MergeCurrentIrelandHourly(CTRef TRef, double elev, const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);
		ERMsg SplitCurrentIrelandDaily(CTRef TRef, const std::string& inputFilePath, CCallback& callback);
		ERMsg ReadIrelandData(const std::string& filePath, CWeatherStation& data, CCallback& callback)const;
		//ERMsg ReadDataHistorical(const std::string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const;
		


		std::bitset<NB_NETWORKS> GetNetworks()const;

		std::string GetStationListFilePath(size_t n)const;

		//ERMsg GetFileList(size_t n, CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;
		//ERMsg UpdateStationsList(size_t n, StringVector& fileList, CCallback& callback)const;
		ERMsg GetStationList(size_t n, size_t type, StringVector& fileList, CCallback& callback)const;
		StringVector CleanIrelandList(size_t n, size_t type, const StringVector& stationList);
		std::string GetOutputFilePath(size_t n, size_t type, const std::string& stationName, int year=-999)const;

		
		std::array<CLocationVector, NB_NETWORKS> m_stations;
		


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		
		static const char* SERVER_NAME[NB_NETWORKS];

		static const char* NETWORKS_NAME[NB_NETWORKS];
		static const char* NETWORKS_ID[NB_NETWORKS];
		static size_t GetIrelandColumn(const std::string& header);
		static std::vector<size_t> GetIrelandColumns(const StringVector& header);
		static double Convert(HOURLY_DATA::TVarH v, double value, double elev, bool bHourly);
	};

}