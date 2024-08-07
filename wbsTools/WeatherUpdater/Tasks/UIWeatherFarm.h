#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class CUIWeatherFarm : public CTaskBase
	{
	public:
		
		
		enum TDateRange { DR_TODAY, DR_YESTERFAY, DR_LAST_2_DAYS, DR_LAST_5_DAYS, NB_DATE_RANGES };
		//static const char* DATE_RANGE_NAME[NB_DATE_RANGES];
		


		enum TAttributes { MEMBER_NAME, PASSWORD, WORKING_DIR, PROVINCE, DATE_RANGE_TYPE, FORCE_UPDATE_STATIONS_LIST, NB_ATTRIBUTES };
		static size_t GetNetwork(const std::string& network);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIWeatherFarm); }

		CUIWeatherFarm(void);
		virtual ~CUIWeatherFarm(void);


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
		

		ERMsg DownloadStationsList(CLocationVector& stationList, CCallback& callback);

		std::string GetStationsListFilePath()const;
		ERMsg GetAgriFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(const std::string& stationName, int year)const;

		CLocationVector m_stations;

		static CTRef GetTRef(std::string str);
		static bool IsValid(HOURLY_DATA::TVarH v, double value);


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;

	};

}