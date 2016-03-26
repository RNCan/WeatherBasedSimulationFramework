#pragma once

#include "ToolsBase.h"
#include "UIWeather.h"
#include "statistic.h"

//typedef CFL::CStatistic CMonthStatistic[4][12];
//typedef CArray<CMonthStatistic> CMonthStatisticArray;

class CBetaDistribution;
class CBetaStat;
class CProbOfPptFreq;
class CAdvancedNormalStation;
//**************************************************************
class CCompareDailyData: public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT1, INTPUT2, OUTPUT, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT1=CToolsBase::I_NB_ATTRIBUTE, I_INPUT2, I_OUTPUT, I_NB_ATTRIBUTE};


	CCompareDailyData(void);
	virtual ~CCompareDailyData(void);
	CCompareDailyData(const CCompareDailyData& in);

	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);

	void Reset();
	CCompareDailyData& operator =(const CCompareDailyData& in);
	bool operator ==(const CCompareDailyData& in)const;
	bool operator !=(const CCompareDailyData& in)const;

	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);

	
	virtual CString GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

protected:

	CString m_inputFilePath1;
	CString m_inputFilePath2;
	CString m_ouputFilePath;

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }

};

