#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	//**************************************************************
	class CUICIPRA : public CTaskBase
	{
	public:

		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUICIPRA); }

		CUICIPRA(void);
		virtual ~CUICIPRA(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual bool IsHourly()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationsFilePath();
		std::string GetMissingFilePath();

		ERMsg LoadStations(CCallback& callback);
		ERMsg UpdateStationsFile(CCallback& callback);
		std::string GetOutputFilePath(size_t t, int year, const std::string& name);
		ERMsg ReadDataFile(const std::string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const;

		CLocationVector m_stations;
		
		
		std::string GetStationName(const std::string& ID)const;
		static int GetStationIndex(const StringVector& allFilePath, const std::string& stationName);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SUB_DIR;
	};

}