#pragma once

#include "ToolsBase.h"
#include "UIWeather.h"
#include "statistic.h"

class CClimaticChange;
//typedef CFL::CStatistic CMonthStatistic[4][12];
//typedef CArray<CMonthStatistic> CMonthStatisticArray;

//**************************************************************
class CDailyDBFromHourly: public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT, OUTPUT, DELETE_OLD_DB, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT=CToolsBase::I_NB_ATTRIBUTE, I_OUTPUT, I_DELETE_OLD_DB, I_FIRST_YEAR, I_LAST_YEAR, I_NB_ATTRIBUTE};


	CDailyDBFromHourly(void);
	virtual ~CDailyDBFromHourly(void);
	CDailyDBFromHourly(const CDailyDBFromHourly& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CDailyDBFromHourly& operator =(const CDailyDBFromHourly& in);
	bool operator ==(const CDailyDBFromHourly& in)const;
	bool operator !=(const CDailyDBFromHourly& in)const;

	//virtual operator
	virtual std::string GetClassID()const{ return CLASS_NAME; }
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);

protected:

	std::string m_inputFilePath;
	std::string m_outputFilePath;
	bool m_bDeleteOldDB;
	short m_firstYear;
	short m_lastYear;
	

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;


	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }

};

