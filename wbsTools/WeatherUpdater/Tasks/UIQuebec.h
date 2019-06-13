#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "SOPFEU.h"
#include "MDDELCC.h"
#include "MFFP.h"
#include "mosa.h"

namespace WBSF
{

	//**************************************************************
	class CUIQuebec : public CTaskBase
	{
	public:

		enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE };
		enum TNetwork{ SOPFEU, MDDELCC, HYDRO, MFFP, ALCAN, FADQ, SM, NB_NETWORKS };
		enum Tattributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, NETWORK, DATA_TYPE, UPDATE_UNTIL, UPDATE_STATIONS_LIST, USER_NAME_SOPFEU, PASSWORD_SOPFEU, USER_NAME_MFFP, PASSWORD_MFFP, NB_ATTRIBUTES };

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

		std::string GetWorkingDir(size_t n)const;

	protected:

		
		std::bitset<NB_NETWORKS> GetNetwork()const;
	

		void InitSOPFEU(CSOPFEU& obj)const;
		void InitMDDELCC(CMDDELCC& obj)const;
		bool InitMFFP(CMFFP& obj)const;
		
		void Init(size_t n);
		ERMsg GetWeatherStation(size_t n, const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback);
		ERMsg GetStationList(size_t n, StringVector& stationList, CCallback& callback);
		ERMsg Execute(size_t n, CCallback& callback);
		std::string GetOutputFilePath(size_t n, std::string id, int year)const;
		ERMsg CreateWeatherStationQuebec(CCallback& callback);

		CSOPFEU m_SOPFEU;
		CMDDELCC m_MDDELCC;
		CMFFP m_MFFP;
		
		
		int m_firstYear;
		int m_lastYear;
		CLocationVector m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* NETWORK_NAME[NB_NETWORKS];
		static const char* NETWORK_TILE[NB_NETWORKS];
		
		
	};



}

