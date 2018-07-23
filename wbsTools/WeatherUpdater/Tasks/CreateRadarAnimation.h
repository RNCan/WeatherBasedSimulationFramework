#pragma once

#include "TaskBase.h"

namespace WBSF
{

	class CCreateRadarAnimation : public CTaskBase
	{
	public:
		
		enum TAttributes { INPUT_DIR, OUTPUT_DIR, START_DATE, END_DATE, FIRST_HOUR, LAST_HOUR, FRAME_DELAY, LOOP, CREATE_ALL, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCreateRadarAnimation); }

		CCreateRadarAnimation(void);
		virtual ~CCreateRadarAnimation(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;
		CTPeriod GetPeriod()const;
		CTRef GetTRef(const std::string& filePath );
		std::string GetAnimationFilePath(CTRef TRef)const;


	protected:


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];

		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

	};
}