#pragma once


#include "TaskBase.h"

namespace WBSF
{


	class CZipUnzip : public CTaskBase
	{
	public:

		enum TCommand{ ZIP, UNZIP, NB_COMMANDS };
		enum TAttributes { COMMAND, ZIP_FILEPATH, DIRECTORY, FILTER, COPY_SUB_DIRECTORY, SHOW_PROGRSS, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CZipUnzip); }

		CZipUnzip(void);
		virtual ~CZipUnzip(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		ERMsg Zip(CCallback& callback);
		ERMsg Unzip(CCallback& callback);


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
	};
}