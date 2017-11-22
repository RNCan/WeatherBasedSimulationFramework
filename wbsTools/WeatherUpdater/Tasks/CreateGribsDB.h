#pragma once

#include "TaskBase.h"

namespace WBSF
{
	class CClimaticChange;

	//**************************************************************
	class CCreateGribsDB : public CTaskBase
	{
	public:

		enum TATTRIBUTE { INPUT1, INPUT2, INPUT3, FORECAST1, FORECAST2, FORECAST3, OUTPUT, FIRST_DATE, LAST_DATE, NB_ATTRIBUTES };
		
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCreateGribsDB); }

		CCreateGribsDB(void);
		virtual ~CCreateGribsDB(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		//virtual bool IsCreator()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		CTPeriod GetPeriod()const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};

}