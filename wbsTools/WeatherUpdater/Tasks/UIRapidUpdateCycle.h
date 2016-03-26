#pragma once

#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{


	//**************************************************************
	class CUIRapidUpdateCycle : public CTaskBase
	{

	public:

		enum TAttributes { WORKING_DIR, BEGIN, END, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIRapidUpdateCycle); }

		CUIRapidUpdateCycle(void);
		virtual ~CUIRapidUpdateCycle(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		//virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:


		ERMsg DownloadGrib(UtilWWW::CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, CCallback& callback)const;
		bool NeedDownload(const std::string& filePath)const;

		std::string GetInputFilePath(CTRef TRef, bool bGrib)const;
		std::string GetOutputFilePath(CTRef TRef, bool bGrib)const;

		//std::string m_path;
		//std::string m_period;

		CTPeriod GetPeriod()const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const StringVector ATTRIBUTE_TITLE;
		static const char* SERVER_NAME;
		static const char* INPUT_FORMAT1;
		static const char* INPUT_FORMAT2;
		static const char* INPUT_FORMAT3;
		static const char* INPUT_FORMAT4;

	};

}