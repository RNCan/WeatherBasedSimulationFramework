#pragma once


#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{
	class CUIHRDPA : public CTaskBase
	{
	public:

		enum{ TYPE_06HOURS, TYPE_24HOURS };
		enum TProduct { RDPA, HRDPA, NB_PRODUCTS };
		enum TAttributes { WORKING_DIR, PRODUCT, TYPE, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIHRDPA); }

		CUIHRDPA(void);
		virtual ~CUIHRDPA(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDaily()const { return true; }
		virtual bool IsGribs()const { return true; }


		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

	//	ERMsg DownloadForecast(CCallback& callback);
		std::string GetOutputFilePath(const std::string& fileName)const;
		bool NeedDownload(const CFileInfo& info, const std::string& filePath)const;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}