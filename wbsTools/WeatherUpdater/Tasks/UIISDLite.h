#pragma once


#include "StateSelection.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{

	class CShapeFileBase;
	//**************************************************************
	class CUIISDLite : public CTaskBase
	{
	public:

		enum TField{ ISD_YEAR, ISD_MONTH, ISD_DAY, ISD_HOUR, ISD_T, ISD_TDEW, ISD_P, ISD_WDIR, ISD_WSPD, ISD_SKY, ISD_PRCP1, ISD_PRCP6, NB_ISD_FIELD };
		typedef std::array<float, CUIISDLite::NB_ISD_FIELD> FieldArray;


		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, PROVINCES, NB_ATTRIBUTES };
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


		ERMsg UpdateStationHistory(CCallback& callback)const;
		

	protected:


		std::string GetHistoryFilePath(bool bLocal = true)const;
		ERMsg ExtractCountrySubDivision(const std::string& txtFilePath, const std::string& csvFilePath, CCallback& callback)const;
		double GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII)const;
		CLocation LocationFromLine(const StringVector& line)const;

		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback)const;
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback)const;

		ERMsg ReadData(const std::string& filePath, CWeatherStation& station, CWeatherAccumulator& stat, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(const std::string& stationName, short year, const std::string& ext = ".gz")const;
		bool IsFileInclude(const std::string& fileTitle)const;
		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;

		
		CLocationMap m_stations;
		
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