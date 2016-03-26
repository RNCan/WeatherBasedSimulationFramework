#pragma once

#include "UIWeather.h"
#include "UtilStd.h"



//**************************************************************
class CUI_MDDEPDaily: public CUIWeather
{
public:

	//force type
	//enum TForce { NEVER, AFTER_ONE_YEAR, SEPTEMBER_2007, BLANK_PAGE, INCOMPLETE_PAGE, ALWAYS, NB_FORCE};
	
	enum TATTRIBUTE { BOUNDINGBOX, FORCE_DOWNLOAD, EXTRACT_SNOW, NB_ATTRIBUTE };
	enum TATTRIBUTE_I { I_BOUNDINGBOX = CUIWeather::I_NB_ATTRIBUTE, I_FORCE_DOWNLOAD, I_EXTRACT_SNOW, I_NB_ATTRIBUTE };


	CUI_MDDEPDaily(void);
	virtual ~CUI_MDDEPDaily(void);
	CUI_MDDEPDaily(const CUI_MDDEPDaily& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUI_MDDEPDaily& operator =(const CUI_MDDEPDaily& in);
	bool operator ==(const CUI_MDDEPDaily& in)const;
	bool operator !=(const CUI_MDDEPDaily& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	virtual ERMsg PreProcess(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(StringVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const std::string& stationName, CDailyStation& station, CFL::CCallback& callback=DEFAULT_CALLBACK);
	

	ERMsg GetStationList(CLocationMap& stationList, CFL::CCallback& callback)const;
	ERMsg CleanStationList(CLocationVector& stationList, CFL::CCallback& callback)const;

protected:


	ERMsg DownloadStation( UtilWWW::CHttpConnectionPtr& pConnection, const CLocation& info, CFL::CCallback& callback);
	
	std::string GetOutputFilePath(int year, int month, const std::string& stationName)const;
	
	ERMsg ReadData(const std::string& filePath, CYear& dailyData)const;
	void GetStationInformation(const std::string& ID, CLocation& station)const;
	

	//Update station list part
	//ERMsg UpdateCoordinate(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& ID, int year, int m, CLocation& station)const;
	


	//Update data part
	ERMsg ParseStationDataPage(const std::string& sourceIn, CLocation& station, std::string& parsedText )const;
	static long GetNbDay(const CTime& t);
	static long GetNbDay(int y, int m, int d);

	bool NeedDownload(const std::string& filePath, const CLocation& info, int y, int m)const;
	ERMsg CopyStationDataPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& ID, int year, int m, const std::string& filePath);
	
	std::string GetStationListFilePath()const;
	

	
	
	GeoBasic::CGeoRect m_boundingBox;
	short m_bUpdateStationList;
	bool m_bExtractSnow;

	CLocationMap m_stations;
	

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;
	static const char* SERVER_PATH;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	//static CTPeriod String2Period(std::string period);
	
};

