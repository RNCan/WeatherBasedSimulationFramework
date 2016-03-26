#pragma once

#include "TaskBase.h"

namespace WBSF
{

	//**************************************************************
	class CCreateHourlyDB : public CTaskBase
	{
	public:

		enum TATTRIBUTE { INPUT, FORECAST, OUTPUT_FILEPATH, FIRST_YEAR, LAST_YEAR, BOUNDING_BOX, DAILY_COMPLETENESS, MONTHLY_COMPLETENESS, ANNUAL_COMPLETENESS, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCreateHourlyDB); }

		CCreateHourlyDB(void);
		virtual ~CCreateHourlyDB(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual bool IsCreator()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual const std::string& Title(size_t i)const{ ASSERT(i<NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const StringVector ATTRIBUTE_TITLE;


		void CleanSparse(CWeatherStation& station)const;
		ERMsg CreateDatabase(CTaskBase& weatherUpdater, CCallback& callback = DEFAULT_CALLBACK)const;
	};

}