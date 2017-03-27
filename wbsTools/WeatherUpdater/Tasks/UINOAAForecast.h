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

		enum TAttributes  { WORKING_DIR, NB_ATTRIBUTES };
		enum TVars{ V_TAIR, V_TDEW, V_RELH, V_WNDS, V_WNDD, NB_VARS };

		static const char* VAR_FILE_NAME[NB_VARS];
		static const HOURLY_DATA::TVarH FORECAST_VARIABLES[NB_VARS];



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
		std::array<CGDALDatasetEx, NB_VARS> m_datasets;
		CProjectionTransformation m_geo2gribs;

		ERMsg OpenDatasets(CCallback& callback);

		std::string GetOutputFilePath(const std::string& str);


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};
}