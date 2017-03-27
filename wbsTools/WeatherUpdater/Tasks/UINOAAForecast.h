#pragma once

#include <bitset>
#include <array>


#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "UI/Common/UtilWWW.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"

#include "TaskBase.h"

namespace WBSF
{
	class CUINOAAForecast : public CTaskBase
	{
	public:

		enum TDataType  { DATA_HOURLY, DATA_DAILY, NB_DATA_TYPE };
		enum TForecastType  { SHORT_FORECAST, LONG_FORECAST, NB_FORECAST_TYPE };
		enum TAttributes  { WORKING_DIR, NB_ATTRIBUTES };
		enum THourlyVars{ V_TAIR, V_TDEW, V_RELH, V_WNDS, V_WNDD, NB_HOURLY_VARS };
		enum TDailyVars{ V_TMIN, V_TMAX, V_PRCP, V_HRMN, V_HRMX, NB_DAILY_VARS };
		enum TVars{ NB_VARS_MAX = NB_DAILY_VARS };


		static const size_t NB_VARS[NB_DATA_TYPE];
		
		static const char* VAR_FILE_NAME[NB_DATA_TYPE][NB_VARS_MAX];
		static const char* FORECAST_TYPE[NB_FORECAST_TYPE];
		static const HOURLY_DATA::TVarH FORECAST_VARIABLES[NB_DATA_TYPE][NB_VARS_MAX];



		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUINOAAForecast); }

		CUINOAAForecast(void);
		virtual ~CUINOAAForecast(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsForecast()const{ return true; }
		virtual bool IsDatabase()const{ return false; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;
		std::string Option(size_t i)const;

	protected:

		bool m_bOpen;
		std::array<std::array<std::array<CGDALDatasetEx, NB_VARS_MAX>, NB_FORECAST_TYPE>, NB_DATA_TYPE> m_datasets;
		CProjectionTransformation m_geo2gribs;
		CGeoExtents m_extents;

		ERMsg OpenDatasets(CCallback& callback);

		static std::string GetInputFilePath(size_t t, size_t f, size_t v);
		std::string GetOutputFilePath(size_t t, size_t f, size_t v)const;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};
}