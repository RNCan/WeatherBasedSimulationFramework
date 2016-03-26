#pragma once

#include "UIWeather.h"
#include ".\StateSelection.h"
#include "station.h"
#include "2dimArray.h"

class CDailyData;
class C24HoursData;



//**************************************************************
class CUSHourly: public CUIWeather
{
public:

	enum TForce { NEVER, AFTER_ONE_YEAR, SEPTEMBER_2007, BLANK_PAGE, INCOMPLETE_PAGE, ALWAYS, NB_FORCE};
	enum TATTRIBUTE {FIRST_MONTH, LAST_MONTH, PROVINCE, BOUNDINGBOX, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_FIRST_MONTH=CUIWeather::I_NB_ATTRIBUTE, I_LAST_MONTH, I_PROVINCE, I_BOUNDINGBOX,  I_NB_ATTRIBUTE};


	CUSHourly(void);
	virtual ~CUSHourly(void);
	CUSHourly(const CUSHourly& in);

	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);

	void Reset();
	CUSHourly& operator =(const CUSHourly& in);
	bool operator ==(const CUSHourly& in)const;
	bool operator !=(const CUSHourly& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	//virtual CUpdaterBase* Copy();

	virtual void GetSelection(short param, CSelectionDlgItemArray& items)const;
	virtual void SetSelection(short param, const CSelectionDlgItemArray& items);
	
	virtual CString GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CSCCallBack& callback=DEFAULT_CALLBACK);
	
	
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	virtual ERMsg PreProcess(CSCCallBack& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(CStringArray& stationList, CSCCallBack& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const CString& stationName, CDailyStation& station);


protected:

	CStateSelection m_selection;
	short m_firstMonth;
	short m_lastMonth;

	CGeoRect m_boundingBox;
	short m_type;
	
	CStationArray m_stationList;

	

	//Database Creation part
	void GetStationHeader( const CString& stationName, CDailyStation& station);
	ERMsg LoadStationList();
	

	

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

