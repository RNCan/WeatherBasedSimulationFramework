#pragma once

#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{
	//****************************************************************
	class CUINovaScotia : public CTaskBase
	{
	public:

		enum TNetwork{FIRE, NB_NETWORKS};
		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		static size_t GetNetwork(const std::string& network);


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUINovaScotia); }

		CUINovaScotia(void);
		virtual ~CUINovaScotia(void);


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
		

		//ERMsg UpdateStationsFile(CCallback& callback);

		std::string GetStationsListFilePath(size_t network)const;
		std::string GetOutputFilePath(int year, const std::string& ID)const;
		ERMsg UpdateStationList(CCallback& callback);

		static CTRef GetTRef(std::string str);
		ERMsg ReadDataFile(const std::string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const;
		//ERMsg ExecuteFire(CCallback& callback);
		//ERMsg SplitFireData(const std::string& outputFilePath, CCallback& callback);



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