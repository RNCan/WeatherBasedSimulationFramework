#pragma once

#include "UIWeather.h"

class CDailyData;

//**************************************************************
class CUIClimChange: public CUIWeather
{
public:

	
	enum TATTRIBUTE {NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_NB_ATTRIBUTE=CUIWeather::I_NB_ATTRIBUTE};


	CUIClimChange(void);
	virtual ~CUIClimChange(void);
	CUIClimChange(const CUIClimChange& in);

	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);

	void Reset();
	CUIClimChange& operator =(const CUIClimChange& in);
	bool operator ==(const CUIClimChange& in)const;
	bool operator !=(const CUIClimChange& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	//virtual CUpdaterBase* Copy();

	//virtual void GetSelection(short param, CSelectionDlgItemArray& items)const;
	//virtual void SetSelection(short param, const CSelectionDlgItemArray& items);
	
	virtual CString GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CSCCallBack& callback=DEFAULT_CALLBACK);
	
	
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	virtual ERMsg PreProcess(CSCCallBack& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(CStringArray& stationList, CSCCallBack& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const CString& stationName, CDailyStation& station, CSCCallBack& callback=DEFAULT_CALLBACK);

protected:

	//Database Creation part
//	ERMsg ReadStationCoord(const CString& filePath, CDailyStation& station)const;
//	ERMsg ReadDataFile(const CString& filePath, short month, short year, CDailyData& oneYearData)const;


//	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
//	static const char* SERVER_NAME;
	

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

