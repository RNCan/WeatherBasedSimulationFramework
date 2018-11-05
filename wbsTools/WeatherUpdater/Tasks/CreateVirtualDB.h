#pragma once

#include "TaskBase.h"
#include "basic/weatherStation.h"
#include "basic/FileStamp.h"
#include "Geomatic/SfcGribsDatabase.h"

namespace WBSF
{
	

	//**************************************************************
	class CCreateVirtualDB : public CTaskBase
	{
	public:

		enum TNbPoints { E_AT_LOCATION=0};
		enum TOutput{ OT_HOURLY, OT_DAILY, NB_OUTPUT_TYPES };
		enum TATTRIBUTE { INPUT_FILEPATH, LOCATIONS_FILEPATH, OUTPUT_FILEPATH, VARIABLES, NB_POINTS, INCREMENTAL, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CCreateVirtualDB); }


		CCreateVirtualDB(void);
		virtual ~CCreateVirtualDB(void);

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

		//ERMsg ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback);
		static GribVariables GetVariables(std::string str);

	protected:

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

		

		//void CleanSparse(CWeatherStation& station)const;
		//ERMsg CreateDatabase(const std::string& outputFilepath, CTaskPtr& pTask, CTaskPtr& pForecastTask, CCallback& callback = DEFAULT_CALLBACK)const;
	};

}