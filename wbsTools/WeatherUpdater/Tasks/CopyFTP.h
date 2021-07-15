#pragma once

#include "TaskBase.h"

namespace WBSF
{

	class CCopyFTP : public CTaskBase
	{
	public:

		enum TDirection { D_DOWNLOAD, D_UPLOAD, NB_DIRECTIONS };
		//enum TConnection { USE_PRECONFIG, DIRECT_TO_INTERNET, USE_PROXY, NB_CONNECTION_TYPE };
		enum TAttributes { DIRECTION, SERVER, REMOTE, LOCAL, USER_NAME, PASSWORD, CONNECTION_TIMEOUT, PASSIVE, SHOW_PROGRESS, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCopyFTP); }

		CCopyFTP(void);
		virtual ~CCopyFTP(void);


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


	protected:


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];

		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

	};
}