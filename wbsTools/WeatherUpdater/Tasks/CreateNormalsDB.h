#pragma once

#include "TaskBase.h"
#include "Simulation/MonthlyMeanGrid.h"

namespace WBSF
{
	class CClimaticChange;

	//**************************************************************
	class CCreateNormalsDB : public CTaskBase
	{
	public:

		enum TATTRIBUTE { INPUT, OUTPUT, FIRST_YEAR, LAST_YEAR, NB_YEARS_MIN, APPLY_CLIMATIC_CHANGE, MMG_FILEPATH, NORMAL_PERIOD, FUTUR_PERIOD, NB_NEIGHBOR, MAX_DISTANCE, POWER, NB_ATTRIBUTES };
		
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCreateNormalsDB); }

		CCreateNormalsDB(void);
		virtual ~CCreateNormalsDB(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		//virtual void UpdateLanguage();
		//virtual bool IsCreator()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual const std::string& Title(size_t i)const{ ASSERT(i<NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;
		
		

	protected:

		CNormalFromDaily::CCPeriodBitset GetCCPeriod()const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};

}