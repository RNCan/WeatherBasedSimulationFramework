#pragma once

#include "Basic/UtilStd.h"
#include "Basic/WeatherStation.h"
#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{

	class CHRDPS;
	class CHRRR;

	//**************************************************************
	class CUIGribCurrent : public CTaskBase
	{

	public:

		
		enum TProduct { P_HRDPS, P_HRRR, P_RAP, P_NAM, P_NAM_NEST_CONUS, P_WRF_ARW, P_NMMB, P_GFS, NB_PRODUCTS };
		enum TDimension { D_SURFACE, D_3D, NB_DIMENSIONS};
		enum TVariable { V_BIOSIM_VAR, V_ALL, NB_VARIABLES};
		enum TAttributes { WORKING_DIR, PRODUCT, DIMENSION, VARIABLE, COPY_FROM_FORECAST, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGribCurrent); }
		static CTRef GetTRef(std::string filePath);

		CUIGribCurrent(void);
		virtual ~CUIGribCurrent(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsGribs()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)override;

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		
		void GetHRDPS(CHRDPS& HRDPS)const;
		void GetHRRR(CHRRR& HRRR)const;
		ERMsg ExecuteHRDPS(CCallback& callback);
		ERMsg ExecuteHRRR(CCallback& callback);
		
		ERMsg GetFilesToDownload(std::vector<std::pair<std::string, std::string>>& fileList, CCallback& callback);
		

		
		CTPeriod CleanList(std::vector<std::pair<std::string, std::string>>& fileList);
		ERMsg CreateHourlyPrcp(const std::string& inputFilePath1, const std::string& inputFilePath2, const std::string& outputFilePath, CCallback& callback)const;
		ERMsg CreateHourlyGeotiff(const std::string& inputFilePath, const std::string& inputPcppFilePath, CCallback& callback)const;
		

		std::string GetOutputFilePath(size_t product, size_t dimension, CTRef TRef, size_t HH, std::string ext)const;
		std::string GetForecastFilePath(size_t product, size_t dimension, CTRef TRef, size_t HH, std::string ext)const;
		
		
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* PRODUCT_NAME[NB_PRODUCTS];

		static std::string GetRemoteFile(size_t product, std::string file, size_t HH);
		static ERMsg Shift_180(const std::string& inputFilePath, CCallback& callback);
		static std::string GetDirectoryName(size_t product, size_t dimension);
		static ERMsg CreateVRT(const std::string& inputFilePath, const std::string& inputPrcpFilePath, const std::string& file_path_vrt);
		static CTRef GetTRef(size_t s, const std::pair<std::string, std::string>& remote);
		static size_t GetHH(size_t s, const std::pair<std::string, std::string>& remote);
		static std::string GetProductName(size_t product, size_t dimension, bool bName=true);
		static bool IsNeeded(size_t product, const std::pair<std::string, std::string>& remote);
	};

}