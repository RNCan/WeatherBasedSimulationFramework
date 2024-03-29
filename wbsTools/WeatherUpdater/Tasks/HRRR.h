#pragma once



#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{


	//**************************************************************
	class CHRRR
	{
	public:
		
		
		enum TSource { MESO_WEST, NOMADS, NB_SOURCES};
		enum TServer { HTTP_SERVER, FTP_SERVER, NB_SERVER_TYPE };
		enum TProduct { HRRR_SFC, HRRR_3D, NB_PRODUCT };
		
		CHRRR(const std::string& workingDir="");
		virtual ~CHRRR(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ExecuteFTP(CCallback& callback);
		ERMsg ExecuteHTTP(CCallback& callback);
		
		ERMsg ExecuteHistorical(CCallback& callback);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback);
		

		static bool NeedDownload(const std::string& filePath) { return !GoodGrib(filePath); }
		

		size_t m_product;
		size_t m_source;
		size_t m_serverType;
		CTPeriod m_period;
		bool m_bShowWINSCP;
		bool m_compute_prcp;
		size_t m_update_last_n_days;
		bool bUseOnlyBioSIMVar;
		bool m_createDailyGeotiff;
		
		std::string GetWorkingDir()const { return m_workingDir; }
		void SetWorkingDir(const std::string& in){ m_workingDir= in; }
	protected:

		std::string m_workingDir;
		std::string GetOutputFilePath(const std::string& filePath)const;
		std::string GetOutputFilePath(CTRef TRef, size_t HH)const;


		static CTRef GetRemoteTRef(const std::string& filePath);
		static CTRef GetLocalTRef(std::string filePath);

		ERMsg GetFilesToDownload(CFileInfoVector& fileList, CCallback& callback);
		ERMsg CreateHourlyGeotiff(std::set<std::string> date_to_update, CCallback& callback)const;
		ERMsg CreateHourlyGeotiff(const std::string& fileList, CCallback& callback)const;
		ERMsg CreateDailyGeotiff(std::set<std::string> date_to_update, CCallback& callback)const;
		ERMsg CreateDailyGeotiff(StringVector& filesList, const std::string& file_path_out, CCallback& callback)const;
		//std::set<std::string> GetAll(CCallback& callback)const;
		std::set<std::string> Getlast_n_days(size_t nb_days, CCallback& callback);
		//ERMsg CreateVRT(std::set<stdstring> outputPath, CCallback& callback);

		static const char* SERVER_NAME[NB_SOURCES][NB_SERVER_TYPE];
		static const char* SERVER_PATH[NB_SOURCES][NB_SERVER_TYPE];

		static const char* PRODUCT_ABR[NB_SOURCES][NB_PRODUCT];
	};

}