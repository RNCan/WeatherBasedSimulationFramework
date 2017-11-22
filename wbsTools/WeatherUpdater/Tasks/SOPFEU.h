#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	typedef std::map<std::string, CWeatherStation> CWeatherStationMap;

	//**************************************************************
	class CSOPFEU 
	{
	public:


		enum TField{ TAI000H, TAN000H, TAX000H, TAM000H, PC040H, PT040H, PC020H, PT041H, HAI000H, HAN000H, HAX000H, NSI000H, VDI300H, VVI300H, VVXI500H, VDM025B, VVM025B, TDI000H, VB000B, NB_FIELDS };
		static const char * FIELDS_NAME[NB_FIELDS];
		static const HOURLY_DATA::TVarH VARIABLE[NB_FIELDS];
		static HOURLY_DATA::TVarH GetVariable(std::string str);

		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		
		CSOPFEU(void);
		virtual ~CSOPFEU(void);

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);
		std::string GetStationListFilePath()const;
		

		std::string m_workingDir;
		std::string m_password;
		std::string m_userName;
		int m_firstYear;
		int m_lastYear;
		

		
	protected:


		ERMsg LoadWeatherInMemory(CCallback& callback);
		
		ERMsg ReadData(const std::string& filePath, CWeatherStationMap& m_stations, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(CTRef TRef)const;
		

		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback)const;
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback)const;

		CLocationVector m_stationsList;
		CWeatherStationMap m_stations;
		
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;

		static bool IsValid(HOURLY_DATA::TVarH v, double value);
	};



}