#pragma once

#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
class CDailyDatabase;
class CDailyStation;


class CCreateOURANOSNormal : public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT_PATH, OUTPUT_FILE_PATH, PERIOD, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT_PATH=CToolsBase::NB_ATTRIBUTE, I_OUTPUT_FILE_PATH, I_PERIOD, I_NB_ATTRIBUTE};


	CCreateOURANOSNormal(void);
	virtual ~CCreateOURANOSNormal(void);
	CCreateOURANOSNormal(const CCreateOURANOSNormal& in);

	void Reset();
	CCreateOURANOSNormal& operator =(const CCreateOURANOSNormal& in);
	bool operator ==(const CCreateOURANOSNormal& in)const;
	bool operator !=(const CCreateOURANOSNormal& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);


	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}
	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);
	
protected:
	
	CString m_inputPath;
	CString m_outputFilePath;
	short m_period;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

};

