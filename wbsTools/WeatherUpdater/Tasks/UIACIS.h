#pragma once

#include "UIWeather.h"
#include "ProvinceSelection.h"
//#include "EnvCanForecast.h"
#include "TmplEx\SortedKeyArray.h"


//**************************************************************
class CUIACIS: public CUIWeather
{
public:

	enum TData { HOURLY_WEATHER, DAILY_WEATHER, NB_TYPE};
	enum TATTRIBUTE {USER_NAME, PASSWORD, TYPE, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_USER_NAME=CUIWeather::I_NB_ATTRIBUTE, I_PASSWORD, I_TYPE, I_NB_ATTRIBUTE};


	CUIACIS(void);
	virtual ~CUIACIS(void);
	CUIACIS(const CUIACIS& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUIACIS& operator =(const CUIACIS& in);
	bool operator ==(const CUIACIS& in)const;
	bool operator !=(const CUIACIS& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);

	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	virtual ERMsg PreProcess(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(StringVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const std::string& stationName, CDailyStation& station, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CFL::CCallback& callback = DEFAULT_CALLBACK);

	ERMsg GetStationList(CLocationVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK)const;
	//ERMsg UpdateStationList(CFL::CCallback& callback=DEFAULT_CALLBACK);


protected:

	//ERMsg DownloadStationHourly( UtilWWW::CHttpConnectionPtr& pConnection, std::string sessionID, const CLocation& info, CFL::CCallback& callback);
	//ERMsg DownloadStationDaily(UtilWWW::CHttpConnectionPtr& pConnection, std::string sessionID, const CLocation& info, CFL::CCallback& callback);
	ERMsg DownloadStationHourly(CFL::CCallback& callback);
	ERMsg DownloadStationDaily(CFL::CCallback& callback);
	std::string GetOutputFilePath(int year, const std::string& stationID)const;
	std::string GetOutputFilePath(int year, size_t m, const std::string& stationID)const;
	std::string GetForecastFilePath(const std::string& stationID)const;

	bool IsFileInclude(const std::string& fileTitle)const;
	ERMsg CleanList(StringVector& fileList, CFL::CCallback& callback)const;
	ERMsg ReadData(const std::string& filePath, CYear& dailyData, CFL::CCallback& callback)const;
	void GetStationInformation(const std::string& fileTitle, CLocation& station)const;
	

	//Update data part
	ERMsg ParseStationDataPage(const std::string& sourceIn, CWeatherStation& station, std::string& parsedText )const;
	bool IsValid(const CLocation& info, short y, short m, const std::string& filePath )const;
	static long GetNbDay(const CTime& t);
	static long GetNbDay(int y, int m, int d);

	std::string GetStationListFilePath()const{ return (std::string)GetWorkingDir() + "StationsList.csv";}


	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;
	
	long m_nbDays;
	std::string m_userName;
	std::string m_password;
	int m_type;



	//stat
	void InitStat();
	void AddToStat(short year);
	void ReportStat(CFL::CCallback& callback);
	
	CLocationVector m_stations;
	int m_nbDownload;
	CArray<int> m_stat;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

