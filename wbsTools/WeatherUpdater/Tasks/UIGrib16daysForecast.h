#pragma once

#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "EnvCanGribForecast.h"
#include "TaskBase.h"




namespace WBSF
{


	//**************************************************************
	class CUIGrib16daysForecast : public CTaskBase
	{

	public:

		enum TServer { HTTP_SERVER, FTP_SERVER, NB_SERVER_TYPE };
		enum TNetwork{ N_GFS, NB_SOURCES };
		enum TAttributes { WORKING_DIR, SOURCES, SERVER_TYPE, FIRST_HOUR, LAST_HOUR, DELETE_AFTER, SHOW_WINSCP, NB_ATTRIBUTES };


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGrib16daysForecast); }


		CUIGrib16daysForecast(void);
		virtual ~CUIGrib16daysForecast(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const { return true; }
		virtual bool IsForecast()const{ return true; }
		virtual bool IsDatabase()const { return true; }
		virtual bool IsGribs()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual ERMsg Initialize(TType type, CCallback& callback = DEFAULT_CALLBACK)override;
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)override;
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK)override;
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)override;
		virtual ERMsg Finalize(TType type, CCallback& callback = DEFAULT_CALLBACK)override;


		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

		ERMsg ExecuteFTP(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ExecuteHTTP(CCallback& callback = DEFAULT_CALLBACK);

		ERMsg ExtractStation(CTRef TRef, CWeatherStation& station, CCallback& callback);
	protected:

		
		CTRef GetLatestTRef(size_t source, UtilWWW::CFtpConnectionPtr& pConnection)const;
		CTRef GetLatestTRef(size_t source, UtilWWW::CHttpConnectionPtr& pConnection)const;
		
		bool NeedDownload(const std::string& filePath)const { return !GoodGrib(filePath); }
		static std::string GetRemoteFilePath(size_t source, CTRef TRef, size_t HH, size_t hhh);
		//ERMsg GetLatestHH(size_t& HH, CCallback& callback)const;
		static void CompleteVariables(CWeatherStation& weather);
		
		ERMsg Clean(size_t source, CCallback& callback);
		CTPeriod CleanList(size_t s, CFileInfoVector& fileList1);
		ERMsg GetFilesToDownload(size_t source, size_t server, CFileInfoVector& fileList, CCallback& callback);
		std::string GetLocaleFilePath(size_t source, const std::string& remote)const;

		
		ERMsg CreateHourlyGeotiff(const std::string& inputFilePath, CCallback& callback)const;


		//std::string GetLocaleFilePath(size_t source, CTRef TRef, size_t HH)const;
		static size_t GetHH(size_t source, std::string filePath);
		static CTRef GetTRef(size_t source, std::string filePath);
		

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SOURCES_NAME[NB_SOURCES];
		static const char* FTP_SERVER_NAME[NB_SOURCES];
		static const char* HTTP_SERVER_NAME[NB_SOURCES];
		static const char* SERVER_PATH[NB_SOURCES];


		//CGribsMap m_gribsList;
		std::map<CTRef, CSfcDatasetCachedPtr> m_psfcDS;
//		CSfcGribDatabasePtr m_pGrib;

	};

}