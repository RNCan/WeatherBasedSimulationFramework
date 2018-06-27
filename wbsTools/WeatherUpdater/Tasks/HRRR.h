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
		
		enum  TServer { HTTP_SERVER, FTP_SERVER, NB_SERVER_TYPE };
		enum { HRRR_3D, HRRR_2D, NB_SOURCES };
		
		CHRRR(const std::string& workingDir);
		virtual ~CHRRR(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ExecuteFTP(CCallback& callback);
		ERMsg ExecuteHTTP(CCallback& callback);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		size_t m_source;
		size_t m_serverType;
		bool m_bShowWINSCP;

	protected:

		std::string m_workingDir;
		std::string GetOutputFilePath(const std::string& filePath)const;

		CTRef GetTRef(const std::string& filePath)const;
		ERMsg GetFilesToDownload(CFileInfoVector& fileList, CCallback& callback);
		

		static const char* SERVER_NAME[NB_SERVER_TYPE];
		static const char* SERVER_PATH[NB_SERVER_TYPE];

		static const char* NAME[NB_SOURCES];
		static const double MINIMUM_SIZE[NB_SOURCES];
	};

}