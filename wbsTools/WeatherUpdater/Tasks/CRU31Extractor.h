#pragma once

#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
class CDailyDatabase;
class CDailyStation;


class CCRU31Extractor : public CToolsBase
{
public:

	enum TExt { TMIN31, TMAX31, PRCP31, NB_VAR};
	//enum TPeriod{ FISRT_PERIOD=2001, PERIOD_SIZE=30, NB_PERIOD=8};

	//enum TExtractType { FROM_DB, FROM_GRID };
	enum TATTRIBUTE {CRU_PATH, INPUT_FILEPATH, OUTPUT_FILEPATH, FIRST_YEAR, LAST_YEAR, NB_POINT_SEARCH, MAX_DISTANCE, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_CRU_PATH=CToolsBase::I_NB_ATTRIBUTE, I_INPUT_FILEPATH, I_OUTPUT_FILEPATH, I_FIRST_YEAR, I_LAST_YEAR, I_NB_POINT_SEARCH, I_MAX_DISTANCE, I_NB_ATTRIBUTE};

	CCRU31Extractor(void);
	~CCRU31Extractor(void);
	//CCRU31Extractor(const CCRU31Extractor& in);

	void Reset();
	CCRU31Extractor& operator =(const CCRU31Extractor& in);
	bool operator ==(const CCRU31Extractor& in)const;
	bool operator !=(const CCRU31Extractor& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}


	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	
protected:

	

//	ERMsg ExecuteFromDaily(CFL::CCallback& callback=DEFAULT_CALLBACK);
	//ERMsg ExecuteFromNormal(CFL::CCallback& callback=DEFAULT_CALLBACK);
//	ERMsg ExecuteFromGrid(CFL::CCallback& callback=DEFAULT_CALLBACK);

	CString m_CRUPath;
	CString m_inputFilePath;
	CString m_outputFilePath;
	int m_nbPointSearch;
	int m_maxDistance;
	int m_firstYear;
	int m_lastYear;
	//bool m_bDeleteOldDB;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	
	
	static const char* OBS_FILE_NAME[NB_VAR];
	CString GetFilePath( short v);
	
};

