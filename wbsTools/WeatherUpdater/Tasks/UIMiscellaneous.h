#pragma once

#include "GSODStationOptimisation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{


	typedef std::map<int, CWeatherYears> CRussiaStationMap;
	//****************************************************************
	//Global SOD extractor
	class CUIMiscellaneous : public CTaskBase
	{
	public:

		enum TDataset{ CDIAC_RUSSIA, NB_DATASETS };
		enum TAttributes { WORKING_DIR, DATASET, FIRST_YEAR, LAST_YEAR, SHOW_PROGRESS, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIMiscellaneous); }

		CUIMiscellaneous(void);
		virtual ~CUIMiscellaneous(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
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

		std::string GetStationListFilePath()const;
		std::string GetMissingFilePath()const;

		ERMsg LoadStationsLocations(CCallback& callback);
		ERMsg UpdateStationList(UtilWWW::CFtpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const;

		std::string GetLocationsFilePath(size_t dataset, bool bLocal = true)const;

		ERMsg UpdateStationLocations();
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		//bool StationExist(const std::string& fileTitle)const;
		bool GetStationInformation(const std::string& fileTitle, CLocation& station)const;

		std::string GetOutputFilePath(const std::string& stationName, int year)const;
		std::string GetOutputFilePath(const CFileInfo& info)const;

		ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;

		ERMsg FTPDownload(const std::string& server, const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);
		ERMsg Uncompress(const std::string& filePathZip, const std::string& workingDir, CCallback& callback);
		ERMsg LoadRussiaInMemory(CCallback& callback);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_DATASETS];
		static const char* SERVER_PATH[NB_DATASETS];
//		static const char* LOCATION_PATH[NB_DATASETS];
	//	static const char* LOCATION_NAME[NB_DATASETS];


		//locations vector
		CLocationVector m_stations;

		//Russia in memory
		CRussiaStationMap m_RussiaStations;
	};

}