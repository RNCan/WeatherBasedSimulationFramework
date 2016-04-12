#pragma once


#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{
	class CUIEnvCanPrcpRadar : public CTaskBase
	{
	public:

		enum{ TYPE_06HOURS, TYPE_24HOURS };
		enum TAttributes { WORKING_DIR, TYPE, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanPrcpRadar); }

		CUIEnvCanPrcpRadar(void);
		virtual ~CUIEnvCanPrcpRadar(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
	

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		//virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		//virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		ERMsg DownloadForecast(CCallback& callback);
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