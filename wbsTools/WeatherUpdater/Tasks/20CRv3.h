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
	class C20CRv3
	{
	public:
		
		
		C20CRv3(const std::string& workingDir="");
		virtual ~C20CRv3(void);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback);

		int m_first_year;
		int m_last_year;
		size_t m_frequency;
		std::string m_workingDir;
		bool m_show_download;
		CGeoRect m_bounding_box;
		
		std::string GetWorkingDir()const { return m_workingDir; }
		void SetWorkingDir(const std::string& in){ m_workingDir= in; }


	protected:

		
		std::string GetOutputFilePath(const std::string& filePath)const;
		std::string GetOutputFilePath(CTRef TRef, size_t HH)const;


		static CTRef GetRemoteTRef(const std::string& filePath);
		static CTRef GetLocalTRef(std::string filePath);

		ERMsg Download(int year, CCallback& callback)const;
		//ERMsg CreateDailyGeotiff(const std::string& inputFilePath, int year, CCallback& callback)const;
		ERMsg CreateDailyGeotiff(int year, CCallback& callback)const;
		//ERMsg CreateDailyGeotiff(std::set<std::string> date_to_update, CCallback& callback)const;
		//ERMsg CreateDailyGeotiff(StringVector& filesList, const std::string& file_path_out, CCallback& callback)const;

		
	};

}