#pragma once

//#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
//class CDailyDatabase;
//class CDailyStation;


class CQualityControl : public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT_DB, OUTPUT_PATH, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT_DB=CToolsBase::NB_ATTRIBUTE, I_OUTPUT_PATH, I_NB_ATTRIBUTE};


	CQualityControl(void);
	virtual ~CQualityControl(void);
	CQualityControl(const CQualityControl& in);

	void Reset();
	CQualityControl& operator =(const CQualityControl& in);
	bool operator ==(const CQualityControl& in)const;
	bool operator !=(const CQualityControl& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	ERMsg ExecuteDaily(CFL::CCallback& callback=DEFAULT_CALLBACK);
//	ERMsg ExecuteNormal(CFL::CCallback& callback=DEFAULT_CALLBACK);

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}

	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	
protected:

	ERMsg CreateDatabseDaily(CFL::CCallback& callback)const;
	ERMsg CreateDatabaseNormal(CFL::CCallback& callback)const;
	
	CString m_inputFilePath;
	CString m_outputPath;


	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

	static void DrawScaleX(CDC* pDC, int x, int y);
	static void DrawScaleY(CDC* pDC, int x, int y);

};

