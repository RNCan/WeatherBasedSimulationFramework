#pragma once


#include "StateSelection.h"
#include "IDSLiteStationOptimisation.h"
#include "CountrySelection.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

//namespace cctz{ class time_zone; }

namespace WBSF
{

	//**************************************************************
	class CUIISDLite : public CTaskBase
	{
	public:

		enum TField{ ISD_YEAR, ISD_MONTH, ISD_DAY, ISD_HOUR, ISD_T, ISD_TDEW, ISD_P, ISD_WDIR, ISD_WSPD, ISD_SKY, ISD_PRCP1, ISD_PRCP6, NB_ISD_FIELD };
		typedef std::array<float, CUIISDLite::NB_ISD_FIELD> FieldArray;


		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIISDLite); }

		CUIISDLite(void);
		virtual ~CUIISDLite(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
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


		std::string GetHistoryFilePath(bool bLocal = true)const;
		ERMsg LoadOptimisation();
		std::string GetOptFilePath(const std::string& filePath)const;
		ERMsg UpdateOptimisationStationFile(const std::string& workingDir, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CWeatherStation& station, CWeatherAccumulator& stat, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(const std::string& stationName, short year, const std::string& ext = ".gz")const;
		bool IsFileInclude(const std::string& fileTitle)const;
		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;
		bool StationExist(const std::string& fileTitle)const;
		void GetStationInformation(const std::string& fileTitle, CLocation& station)const;


		ERMsg UpdateStationHistory(CCallback& callback);
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback)const;
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback)const;

		CIDSLiteStationOptimisation m_optFile;


		//Database Creation part
		//void GetStationHeader(const std::string& stationName, CLocation& station);
		//ERMsg LoadStationList();
		
		static CTRef GetTRef(const FieldArray& e);
		static bool LoadFields(const std::string& line, FieldArray& e);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
		static const char* LIST_PATH;
	};



}