#pragma once

#include "TaskBase.h"
#include "basic/weatherStation.h"
#include "basic/FileStamp.h"
#include "Geomatic/SfcGribsDatabase.h"

namespace WBSF
{
	

	//**************************************************************
	class CUIQualityControl : public CTaskBase
	{
	public:

		enum TOutput{ OT_HOURLY, OT_DAILY, NB_OUTPUT_TYPES };
		enum TATTRIBUTE { HRDPS_PATH, HRRR_PATH, OUTPUT_PATH, UPDATE_LAST_N_DAYS, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIQualityControl); }


		CUIQualityControl(void);
		virtual ~CUIQualityControl(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		//virtual bool IsCreator()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};

}