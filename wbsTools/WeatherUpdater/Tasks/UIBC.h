#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

//namespace UtilWWW{ class CFtpConnectionPtr; }

namespace WBSF
{
	
	//class CWeatherYear;


	//**************************************************************
	class CUIBC : public CTaskBase
	{
	public:

		enum Tattributes { WORKING_DIR, DATA_TYPE, FIRST_YEAR, LAST_YEAR, UPDATE_STATION_LIST, IGNORE_ENV_CAN, NB_ATTRIBUTES };
		enum TNetwork { AGRI, ARDA, BCH, EC, EC_RAW, ENV_AQN, ENV_ASP, FLNRO_FERN, FLNRO_WMB, FRBC, MoTIe, MoTIm, RTA, NB_NETWORKS };
		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };

		static size_t GetNetwork(const std::string& network_name);
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIBC); }

		CUIBC(void);
		virtual ~CUIBC(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDatabase()const{ return true; }
		virtual bool IsHourly()const{ return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
		virtual bool IsDaily()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationListFilePath()const;
		ERMsg sevenZ(const std::string& filePathZip, const std::string& workingDir, CCallback& callback);

		ERMsg LoadStationList(CCallback& callback);
		ERMsg UpdateStationList(CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherStation& data, CCallback& callback)const;

		CLocationVector m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;


		static const char* SERVER_NAME;
		static const char* MTS_SUB_DIR;
		static const char* MCD_SUB_DIR;

		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* NETWORK_TILE[NB_NETWORKS];
		static const size_t NETWOK_ID_TO_ENUM[20];
		static const char* TYPE_NAME[NB_TYPES];
		static const bool HISTORICAL[NB_NETWORKS][NB_TYPES];
		static const bool ACTIVE[NB_NETWORKS][NB_TYPES];

	};

}