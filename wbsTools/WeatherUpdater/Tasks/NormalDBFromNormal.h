#pragma once

#include "ToolsBase.h"
#include "UIWeather.h"
#include "statistic.h"

//**************************************************************
class CNormalDBFromNormal: public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT, OUTPUT, DELETE_OLD_DB, MMG_FILEPATH, NORMAL_PERIOD, NB_NEIGHBOR, MAX_DISTANCE, POWER, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT=CToolsBase::I_NB_ATTRIBUTE, I_OUTPUT, I_DELETE_OLD_DB, I_MMG_FILEPATH, I_NORMAL_PERIOD, I_NB_NEIGHBOR, I_MAX_DISTANCE, I_POWER, I_NB_ATTRIBUTE};


	CNormalDBFromNormal(void);
	virtual ~CNormalDBFromNormal(void);
	CNormalDBFromNormal(const CNormalDBFromNormal& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CNormalDBFromNormal& operator =(const CNormalDBFromNormal& in);
	bool operator ==(const CNormalDBFromNormal& in)const;
	bool operator !=(const CNormalDBFromNormal& in)const;

	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

protected:

	ERMsg CreateNormalDatabase(const std::string& outputFilePath, CFL::CCallback& callback);
	//ERMsg ApplyClimaticChange(CDailyStation& dailyStation, CClimaticChange& cc, CFL::CCallback& callback);
//	static void CleanUpYears( CDailyStation& dailyStation, short firstYear, short lastYear );
	

	std::string m_inputFilePath;
	std::string m_outputFilePath;
	bool m_bDeleteOldDB;
	
	
	std::string m_MMGFilePath;
	int m_futurPeriod;
	short m_nbNeighbor;
	int m_maxDistance;
	double m_power;

//	enum { N_MIN, N_MAX, N_MEAN, N_PPT, NB_VAR };
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const short NB_DAY_PER_MONTH_MIN;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

