#pragma once

#include "UIWeather.h"
#include "2dimArray.h"
#include "DailyStation.h"

class CDailyData;



//**************************************************************
class CUIBCSnowPillow: public CUIWeather
{
public:

	//force type
	//using CUIWeather::TExtraction;
	enum TATTRIBUTE {PROVINCE, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_PROVINCE=CUIWeather::I_NB_ATTRIBUTE, I_NB_ATTRIBUTE};


	CUIBCSnowPillow(void);
	virtual ~CUIBCSnowPillow(void);
	CUIBCSnowPillow(const CUIBCSnowPillow& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUIBCSnowPillow& operator =(const CUIBCSnowPillow& in);
	bool operator ==(const CUIBCSnowPillow& in)const;
	bool operator !=(const CUIBCSnowPillow& in)const;
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
	virtual ERMsg GetStation(const std::string& fileName, CDailyStation& station, CFL::CCallback& callback=DEFAULT_CALLBACK);
	

protected:

	std::string GetOutputFilePath(const std::string& ID, const std::string ext=".csv")const;
	std::string GetStationListFilePath()const{ return GetWorkingDir() + "StationsList.csv"; }
	ERMsg UpdateStationInformation(CFL::CCallback& callback)const;

	CLocationVector m_stations;


	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

