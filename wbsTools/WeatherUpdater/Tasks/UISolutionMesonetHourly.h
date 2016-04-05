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
	class CUISolutionMesonetHourly : public CTaskBase
	{
	public:

		enum Tattributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, UPDATE_STATION_LIST, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUISolutionMesonetHourly); }

		CUISolutionMesonetHourly(void);
		virtual ~CUISolutionMesonetHourly(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual bool IsHourly()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationListFilePath()const;
		std::string GetMissingFilePath()const;

		ERMsg LoadStationList(CCallback& callback);
		ERMsg UpdateStationList(UtilWWW::CFtpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const;

		CLocationMap m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;


		static const char* SERVER_NAME;
		static const char* MTS_SUB_DIR;
		static const char* MCD_SUB_DIR;

	};

}