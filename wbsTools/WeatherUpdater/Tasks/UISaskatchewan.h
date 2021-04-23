#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class UISaskatchewan : public CTaskBase
	{
	public:

		enum TNetwork{FIRE, NB_NETWORKS};
		enum TAttributes { WORKING_DIR, DOWNLOAD_ARCHIVE, NB_ATTRIBUTES };
		static size_t GetNetwork(const std::string& network);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new UISaskatchewan); }

		UISaskatchewan(void);
		virtual ~UISaskatchewan(void);


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

		std::string GetStationsListFilePath(size_t network)const;
		std::string GetOutputFilePath(size_t network, const std::string& stationName, int year, size_t m = NOT_INIT)const;
		

		ERMsg ExecuteFire(CCallback& callback);
		ERMsg MergeFireData(const std::string& ID, std::string source3, CCallback& callback);

		CLocationVector m_stations;

		

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* SERVER_PATH[NB_NETWORKS];
		
		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* SUBDIR_NAME[NB_NETWORKS];

	};

}