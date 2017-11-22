#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/UtilWWW.h"

namespace cctz{ class time_zone; }

namespace WBSF
{

	//**************************************************************
	class CUIMesoWest : public CTaskBase
	{
	public:

		enum Tattributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, STATES, PROVINCES, ADD_OTHER, IGNORE_COMMON_STATIONS, FORCE_UPDATE_STATIONS_LIST, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIMesoWest); }

		CUIMesoWest(void);
		virtual ~CUIMesoWest(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }

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
		std::string GetOutputFilePath(const std::string& country, const std::string& states, const std::string& ID, int year, size_t m);
		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback)const;
		
		ERMsg ReadData(const std::string& filePath, CTM TM, int year, CWeatherStation& data, CCallback& callback)const;

		CLocationVector m_stations;

		static HOURLY_DATA::TVarH GetVar(const std::string& str);
		static std::vector<HOURLY_DATA::TVarH > GetVariables(const StringVector& header);
		static bool IsUnknownVar(const std::string& str);
		static CTRef GetTRef(const std::string& str);
		static bool IsValid(HOURLY_DATA::TVarH v, double value);
		static CLocationVector GetCommonStations(const CLocationVector& stationListTmp);
		static bool IsCommonStation(const std::string& network);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}