#pragma once

#include "UIWeather.h"
#include "ProvinceSelection.h"

//**************************************************************
class CUIBCPAWS: public CUIWeather
{
public:

	//force type
	enum TATTRIBUTE {NB_ATTRIBUTE};
	enum TATTRIBUTE_I { I_NB_ATTRIBUTE = CUIWeather::I_NB_ATTRIBUTE };


	CUIBCPAWS(void);
	virtual ~CUIBCPAWS(void);
	CUIBCPAWS(const CUIBCPAWS& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUIBCPAWS& operator =(const CUIBCPAWS& in);
	bool operator ==(const CUIBCPAWS& in)const;
	bool operator !=(const CUIBCPAWS& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);

	//virtual void GetSelection(short param, CSelectionDlgItemVector& items)const;
	//virtual void SetSelection(short param, const CSelectionDlgItemVector& items);
	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	virtual ERMsg PreProcess(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(StringVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const std::string& stationName, CDailyStation& station, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CFL::CCallback& callback);

	ERMsg UpdateStationList(CFL::CCallback& callback=DEFAULT_CALLBACK)const;
	//ERMsg UpdateStationList(CFL::CCallback& callback=DEFAULT_CALLBACK);


protected:

	//ERMsg DownloadStation( UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CFL::CCallback& callback);
	//ERMsg DownloadForecast( const CLocationVector& stationList, CFL::CCallback& callback);

	std::string GetOutputFilePath(short year, const std::string& stationName)const;

	//bool IsFileInclude(const std::string& fileTitle)const;
	//ERMsg CleanList(StringVector& fileList, CFL::CCallback& callback)const;
	ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;
	//void GetStationInformation(const std::string& fileTitle, CLocation& station)const;
	

	//Update station list part
	//CTPeriod String2Period(const std::string& period)const;
	//int GetNbStation(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page)const;

	//ERMsg GetStationListPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page, CLocationVector& stationList)const;
	//ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
	//ERMsg UpdateCoordinate(UtilWWW::CHttpConnectionPtr& pConnection, long id, int year, int m, CBCPAWSStation& station)const;
	


	//Update data part
	//ERMsg ParseStationDataPage(const std::string& sourceIn, CLocation& station, std::string& parsedText )const;
	//bool IsValid(const CLocation& info, short y, short m, const std::string& filePath )const;
	//static long GetNbDay(const CTime& t);
	//static long GetNbDay(int y, int m, int d);

	//bool NeedDownload(const std::string& filePath, const CLocation& info, short y)const;
	//ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, long id, int year, const std::string& page);
	//std::string GetForecastListFilePath()const{return CFL::GetApplicationPath()+"ForecastLinkEnvCan.csv";}
	std::string GetStationListFilePath()const{ return GetWorkingDir() + "StationsList.csv";}


	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;

	CLocationVector m_stations;
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

