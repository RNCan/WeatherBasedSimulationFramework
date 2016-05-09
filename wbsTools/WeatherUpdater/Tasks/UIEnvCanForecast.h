#pragma once

#include <bitset>
#include <array>


#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "EnvCanHourlyForecast.h"
#include "EnvCanGribForecast.h"
//#include "Geomatic/ShapeFileBase.h"

#include "TaskBase.h"

namespace WBSF
{
	class CUIEnvCanForecast : public CTaskBase
	{
	public:

		enum TMethos { M_METEO_CODE, M_GRIBS, M_BOTH, NB_METHODS};
		enum TGribsType{ GT_HRDPS, GT_RDPS, NB_GRIBS_TYPE};
		enum TAttributes { WORKING_DIR, METHOD, GRIBS_TYPE, NB_ATTRIBUTES };


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanForecast); }

		CUIEnvCanForecast(void);
		virtual ~CUIEnvCanForecast(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsForecast()const{ return true; }
		virtual bool IsHourly()const{ return true; }


		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;
		std::string Option(size_t i)const;

	protected:


		CEnvCanHourlyForecast m_meteoCode;
		CEnvCanGribForecast m_gribs;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};
}