#pragma once

#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "EnvCanGribForecast.h"
#include "TaskBase.h"




namespace WBSF
{


	//**************************************************************
	class CUIGribForecast : public CTaskBase
	{

	public:

		enum TNeytwork{ N_HRDPS, N_HRRR, N_HRRR_SRF, N_RAP_P, N_RAP_B, N_NAM, NB_SOURCES };
		enum TAttributes { WORKING_DIR, SOURCES, MAX_HOUR, DELETE_AFTER, SHOW_WINSCP, HRDPS_VARS_SFC, HRDPS_VARS_TGL, HRDPS_VARS_ISBL, HRDPS_VARS_OTHERS, TGL_HEIGHTS, ISBL_LEVELS, NB_ATTRIBUTES };


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGribForecast); }


		CUIGribForecast(void);
		virtual ~CUIGribForecast(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsForecast()const{ return true; }
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

		
		CTRef GetLatestTRef(size_t source, UtilWWW::CFtpConnectionPtr& pConnection)const;
		//ERMsg DownloadGrib(UtilWWW::CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, CCallback& callback)const;
		bool NeedDownload(const std::string& filePath)const { return !GoodGrib(filePath); }
		//bool GoodGrib(const std::string& filePath)const;
		//CTPeriod GetPeriod()const;
		static std::string GetRemoteFilePath(size_t source, CTRef TRef, size_t HH);
		
		ERMsg Clean(size_t source, CCallback& callback);
		CTPeriod CleanList(size_t s, CFileInfoVector& fileList1);
		ERMsg GetFilesToDownload(size_t source, CFileInfoVector& fileList, CCallback& callback);
		std::string GetLocaleFilePath(size_t source, const std::string& remote)const;
		//std::string GetLocaleFilePath(size_t source, CTRef TRef, size_t HH)const;
		static size_t GetHH(size_t source, std::string filePath);
		static CTRef GetTRef(size_t source, std::string filePath);
		

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SOURCES_NAME[NB_SOURCES];
		static const char* SERVER_NAME[NB_SOURCES];
		static const char* SERVER_PATH[NB_SOURCES];
		
	};

}