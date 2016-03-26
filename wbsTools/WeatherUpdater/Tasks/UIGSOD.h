#pragma once

#include "GSODStationOptimisation.h"
//#include "CountrySelection.h"
//#include "StateSelection.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{


	//class CWeatherInfoSection;
//	class CDailyData;


	//****************************************************************
	//Global SOD extractor
	class CUIGSOD : public CTaskBase
	{
	public:

		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGSOD); }

		CUIGSOD(void);
		virtual ~CUIGSOD(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationListFilePath()const;
		std::string GetMissingFilePath()const;

		ERMsg LoadStationList(CCallback& callback);
		ERMsg UpdateStationList(UtilWWW::CFtpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const;

		CLocationMap m_stations;


		std::string GetHistoryFilePath(bool bLocal = true)const{ return (bLocal ? GetDir(WORKING_DIR) : std::string(LIST_PATH)) + "isd-history.txt"; }

		ERMsg UpdateStationHistory();
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		ERMsg UpdateOptimisationStationFile(const std::string& workingDir, CCallback& callBack = DEFAULT_CALLBACK)const;
		ERMsg UpdateOptimisationDataFile(const std::string& workingDir, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOptFilePath(const std::string& filePath)const;

		ERMsg LoadOptimisation();
		bool StationExist(const std::string& fileTitle)const;
		void GetStationInformation(const std::string& fileTitle, CLocation& station)const;

		//Get stations list part
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		std::string GetOutputFilePath(const std::string& stationName, short year)const;
		std::string GetOutputFilePath(const CFileInfo& info)const;

		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;
		bool IsFileInclude(const std::string& fileTitle)const;
		ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;


		//CCountrySelection m_countries;
		//CStateSelection m_states;
		//CGeoRect m_boundingBox;
		//short m_type;

		//optimisation for stations
		CGSODStationOptimisation m_optFile;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const StringVector ATTRIBUTE_TITLE;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
		static const char* LIST_PATH;

	};

}