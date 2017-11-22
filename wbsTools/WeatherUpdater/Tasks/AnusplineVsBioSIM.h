#pragma once

#include "ToolsBase.h"
#include "statistic.h"


//**************************************************************
class CAnusplineVsBioSIM: public CToolsBase
{
public:

	enum TATTRIBUTE {NORMAL_DB, LOC_INPUT, OUTPUT, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_NORMAL_DB=CToolsBase::I_NB_ATTRIBUTE, I_LOC_INPUT, I_OUTPUT, I_NB_ATTRIBUTE};

	CAnusplineVsBioSIM(void);
	virtual ~CAnusplineVsBioSIM(void);
	CAnusplineVsBioSIM(const CAnusplineVsBioSIM& in);

	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);

	void Reset();
	CAnusplineVsBioSIM& operator =(const CAnusplineVsBioSIM& in);
	bool operator ==(const CAnusplineVsBioSIM& in)const;
	bool operator !=(const CAnusplineVsBioSIM& in)const;

	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);

	
	virtual CString GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

protected:

	ERMsg ExecuteAnuspline(CFL::CCallback& callback);
	ERMsg ExecuteBioSIM(CFL::CCallback& callback);
//2	ERmsg MakeXValidation();

	CString m_normalFilePath;
	CString m_locFilePath;
	CString m_outputFilePath;


	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

