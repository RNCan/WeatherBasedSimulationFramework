#pragma once

#include "GSODStationOptimisation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{


	typedef std::map<std::string, CWeatherYears> CMiWeatherStationMap;
	//****************************************************************
	//Global SOD extractor
	class CUIMiscellaneous : public CTaskBase
	{
	public:

		enum TDataset{ CDIAC_RUSSIA, SOPFEU_HISTORICAL, QUEBEC_HOURLY, CWEEDS, RCM4_22, CWFIS, NB_DATASETS };
		enum TAttributes { WORKING_DIR, DATASET, FIRST_YEAR, LAST_YEAR, SHOW_PROGRESS, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIMiscellaneous); }

		CUIMiscellaneous(void);
		virtual ~CUIMiscellaneous(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return as<size_t>(DATASET) != CDIAC_RUSSIA; }
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


		ERMsg LoadStationsLocations(CCallback& callback);

		std::string GetLocationsFilePath(size_t dataset, bool bLocal = true)const;

		ERMsg UpdateStationLocations();
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		bool GetStationInformation(const std::string& fileTitle, CLocation& station)const;

		std::string GetOutputFilePath(const std::string& stationName, int year)const;
		std::string GetOutputFilePath(const CFileInfo& info)const;

		//ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;

		ERMsg FTPDownload(const std::string& server, const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);
		ERMsg Uncompress(const std::string& filePathZip, const std::string& workingDir, CCallback& callback);
		ERMsg LoadRussiaInMemory(CCallback& callback);
		ERMsg LoadSOPFEUInMemory(CCallback& callback);
		ERMsg LoadQuebecInMemory(CCallback& callback);
		ERMsg LoadCWFISInMemory(CCallback& callback);
		

		static double ConvertMTSData(size_t v, double value);
		ERMsg ReadMTSData(const std::string& filePath, CWeatherYears& data, CCallback& callback)const;
		static bool IsMTSValid(size_t v, double value);
		ERMsg ReadCWEEDSData(const std::string& filePath, CWeatherStation& station)const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_DATASETS];
		static const char* SERVER_PATH[NB_DATASETS];

		//locations vector
		CLocationVector m_stations;

		//Russia or SOPFEU in memory
		CMiWeatherStationMap m_weatherStations;

		std::map<std::string, CLocationVector> SOPFEU_stations;
		
	};

}