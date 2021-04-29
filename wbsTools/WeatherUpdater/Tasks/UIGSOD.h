#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	class CShapeFileBase;
	//****************************************************************
	//Global SOD extractor
	class CUIGSOD : public CTaskBase
	{
	public:

		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, SHOW_PROGRESS, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGSOD); }

		CUIGSOD(void);
		virtual ~CUIGSOD(void);


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

		CLocationMap m_stations;


		std::string GetHistoryFilePath(bool bLocal = true)const{ return (bLocal ? GetDir(WORKING_DIR) : std::string(LIST_PATH)) + "isd-history.csv"; }
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		//Get stations list part
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		std::string GetOutputFilePath(const std::string& stationName, int year)const;
		std::string GetOutputFilePath(const CFileInfo& info)const;

		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;
		bool IsFileInclude(const std::string& fileTitle)const;
		ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;

		ERMsg FTPDownload(const std::string& server, const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);
		ERMsg Uncompress(const std::string& filePathZip, const std::string& workingDir, CCallback& callback);


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
		static const char* LIST_PATH;

	};

}