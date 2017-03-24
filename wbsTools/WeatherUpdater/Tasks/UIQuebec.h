#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "SOPFEU.h"
#include "MDDELCC.h"

namespace WBSF
{

	//**************************************************************
	class CUIQuebec : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TNetwork{ SOPFEU, MDDELCC, HYDRO, MFFP, ALCAN, FADQ, NB_NETWORKS };//METEO_CENTRE, 
		enum Tattributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NETWORK, DATA_TYPE, UPDATE_UNTIL, UPDATE_STATIONS_LIST, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIQuebec); }

		CUIQuebec(void);
		virtual ~CUIQuebec(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsHourly()const{ return as<size_t>(DATA_TYPE) == HOURLY_WEATHER; }
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

		//size_t GetNetwork(const string& network)const;
		std::bitset<NB_NETWORKS> GetNetwork()const;
	//	std::string GetStationListFilePath(size_t n);

		void InitSOPFEU(CSOPFEU& obj)const;
		void InitMDDELCC(CMDDELCC& obj)const;

//		CSOPFEU GetSOPFEU()const;
	//	CMDDELCC GetMDDELCC()const;

		CSOPFEU m_SOPFEU;
		CMDDELCC m_MDDELCC;


//		CLocationVector m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* NETWORK_TILE[NB_NETWORKS];
		
	};



}

