#pragma once

#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{


	//**************************************************************
	class CUINAM : public CTaskBase
	{

	public:

		enum TSource { S_NOMADS, S_NCEP, NB_SOURCES };
		enum TServer { HTTP_SERVER, FTP_SERVER, NB_SERVER_TYPE };
		enum TAttributes { WORKING_DIR, SOURCES, SERVER_TYPE, FIRST_DATE, LAST_DATE, SHOW_WINSCP, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUINAM); }
		static CTRef GetTRef(std::string filePath);

		CUINAM(void);
		virtual ~CUINAM(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsGribs()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)override;

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		ERMsg ExecuteHTTP(CCallback& callback);
		ERMsg ExecuteFTP(CCallback& callback);

		
		ERMsg GetFilesToDownload(size_t s, CFileInfoVector& fileList, CCallback& callback);
		ERMsg DownloadGrib(UtilWWW::CHttpConnectionPtr& pConnection, CTRef TRef, size_t& nbDownloaded, CCallback& callback)const;
		bool NeedDownload(const std::string& filePath)const { return !GoodGrib(filePath); }
		//bool GoodGrib(const std::string& filePath)const;
		CTPeriod CleanList(size_t s, CFileInfoVector& fileList);
		CTRef GetTRef(size_t s, const std::string& fileList);
		bool server_available(size_t s)const;

		std::string GetInputFilePath(CTRef TRef)const;
		std::string GetOutputFilePath(CTRef TRef)const;
		
	
		CTPeriod GetPeriod()const;
		std::bitset<NB_SOURCES> GetSources()const;
		
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

		static const char* HTTP_SERVER_NAME[NB_SOURCES];
		static const char* FTP_SERVER_NAME[NB_SOURCES];
		static const char* NAME_NET[NB_SOURCES];
	};

}