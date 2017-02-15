#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{


	//**************************************************************
	class CUIMDDELCC : public CTaskBase
	{
	public:

		enum Tattributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, UPDATE_UNTIL, UPDATE_STATIONS_LIST, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIMDDELCC); }

		CUIMDDELCC(void);
		virtual ~CUIMDDELCC(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
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
		std::string GetOutputFilePath(int year, size_t m, const std::string& stationName)const;


		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback)const;
		ERMsg DownloadStation(UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback);
		ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& ID, int year, size_t m, const std::string& filePath);
		

		ERMsg ReadData(const std::string& filePath, CWeatherYear& dailyData)const;
		
		

		CLocationVector m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;

		//static const char* NETWORK_NAME[NB_NETWORKS];
		//static const char* NETWORK_TILE[NB_NETWORKS];
		//static const size_t NETWOK_ID_TO_ENUM[20];
		//static const char* TYPE_NAME[NB_TYPES];
		//static const bool AVAILABILITY[NB_NETWORKS][NB_TYPES];
	};



}

