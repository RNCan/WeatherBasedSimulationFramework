#pragma once

#include "WeatherCreator.h"
#include "UIWeather.h"



//**************************************************************
class CNormalDBCreator: public CWeatherCreator
{
public:

	enum TATTRIBUTE {NB_YEAR, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_NB_YEAR=CWeatherCreator::I_NB_ATTRIBUTE, I_NB_ATTRIBUTE};


	CNormalDBCreator(void);
	virtual ~CNormalDBCreator(void);
	CNormalDBCreator(const CNormalDBCreator& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CNormalDBCreator& operator =(const CNormalDBCreator& in);
	bool operator ==(const CNormalDBCreator& in)const;
	bool operator !=(const CNormalDBCreator& in)const;

	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	ERMsg CreateDatabase(CUIWeather& weatherUpdater, CFL::CCallback& callback=DEFAULT_CALLBACK)const;
	
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

protected:

	short m_nbYearMin;

//	enum { N_MIN, N_MAX, N_MEAN, N_PPT, NB_VAR };
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const short NB_DAY_PER_MONTH_MIN;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }

	//ERMsg GetNormalStation(CDailyStation& dailyStation, CNormalsStation& normalStation, int nbYearMinimum)const;
	//ERMsg GetNormalValidity(CDailyStation& dailyStation, int nbYearMin, bool& bValidTemperature, bool& bValidPrecipitation)const;
	//void GetMonthStatistic( CDailyStation& dailyStation, CMonthStatisticArray& monthStat)const;

};

