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
		enum TProduct { HRRR_3D, HRRR_SFC, NB_PRODUCT };
		
		CHRRR(const std::string& workingDir);
		virtual ~CHRRR(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ExecuteFTP(CCallback& callback);
		ERMsg ExecuteHTTP(CCallback& callback);
		ERMsg ExecuteHistorical(CCallback& callback);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		static bool NeedDownload(const std::string& filePath) { return !GoodGrib(filePath); }
		//static bool GoodGrib(const std::string& file_path);

		size_t m_product;
		size_t m_source;
		size_t m_serverType;
		CTPeriod m_period;
		bool m_bShowWINSCP;
		

	protected:

		std::string m_workingDir;
		std::string GetOutputFilePath(const std::string& filePath)const;
		std::string GetOutputFilePath(CTRef TRef)const;


		CTRef GetTRef(const std::string& filePath)const;
		ERMsg GetFilesToDownload(CFileInfoVector& fileList, CCallback& callback);
		
		
		

		static const char* SERVER_NAME[NB_SOURCES][NB_SERVER_TYPE];
		static const char* SERVER_PATH[NB_SOURCES][NB_SERVER_TYPE];

		static const char* PRODUCT_ABR[NB_SOURCES][NB_PRODUCT];
	};

}