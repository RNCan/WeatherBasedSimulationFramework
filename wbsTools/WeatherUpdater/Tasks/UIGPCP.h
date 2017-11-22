#pragma once


#include "StateSelection.h"
#include "IDSLiteStationOptimisation.h"
#include "CountrySelection.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace cctz{ class time_zone; }

namespace WBSF
{

	//**************************************************************
	class CUIGPCP : public CTaskBase
	{
	public:

		enum TData{ DATA_DAILY, DATA_MONTHLY };
		enum TAttributes { WORKING_DIR, DATA_TYPE, FIRST_YEAR, LAST_YEAR, BOUNDING_BOX, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGPCP); }

		CUIGPCP(void);
		virtual ~CUIGPCP(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:


		ERMsg GetStationListD(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStationD(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		ERMsg GetStationListM(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStationM(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		CTRef m_firstTRef;
		std::deque<ifStream> m_files;

		std::string GetOptFilePath(const std::string& filePath)const;


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;

	};



}