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
	class CUIHighResolutionGribs : public CTaskBase
	{

	public:

		enum TNeytwork{ N_HRDPS, N_HRRR, NB_SOURCES };
		enum TAttributes { WORKING_DIR, SOURCES, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIHighResolutionGribs); }
		static CTRef GetTRef(std::string filePath);
		static size_t GetSourcesIndex(const std::string& name);

		CUIHighResolutionGribs(void);
		virtual ~CUIHighResolutionGribs(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsGribs()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:


		ERMsg DownloadGrib(UtilWWW::CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, CCallback& callback)const;
		bool NeedDownload(const std::string& filePath)const;

		//std::string GetInputFilePath(CTRef TRef, bool bGrib, int forecastH)const;
		//std::string GetOutputFilePath(CTRef TRef, bool bGrib, int forecastH)const;
	
		CTPeriod GetPeriod()const;


		
		
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
//		static const char* SERVER_NAME[NB_SOURCES];
	//	static const char* INPUT_FORMAT[NB_SOURCES];
		static const char* SOURCES_NAME[NB_SOURCES];
		
	};

}