#pragma once

#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
class CDailyDatabase;
class CDailyStation;


class CTYNExtractor : public CToolsBase
{
public:

	enum TModel{HadCM3, PCM, CGCM2, CSIRO2, ECHam4, NB_MODEL};
	enum TSRES{A1FI, A2, B2, B1, NB_SRES};
	enum TExt { TMP, DTR, PPT, NB_EXT};
	enum TPeriod{ FISRT_PERIOD=2001, PERIOD_SIZE=30, NB_PERIOD=8};

	enum TExtractType { FROM_DB, FROM_GRID };
	enum TATTRIBUTE {TYPE, MODEL, SRES, PERIOD, TYN_PATH, INPUT_DB, OUT_FILEPATH, DELETE_OLD_DB, NB_ATTRIBUTE};//SHAPEFILE, BOUNDINGBOX, 
	enum TATTRIBUTE_I {I_EXTRACT_TYPE=I_NB_ATTRIBUTE, I_MODEL, I_SRES, I_PERIOD, I_TYN_PATH, I_INPUT_DB, I_OUT_FILEPATH, I_DELETE_OLD_DB, I_NB_ATTRIBUTE};//I_SHAPEFILE, I_BOUNDINGBOX, 
	

	CTYNExtractor(void);
	virtual ~CTYNExtractor(void);
	CTYNExtractor(const CTYNExtractor& in);

	void Reset();
	CTYNExtractor& operator =(const CTYNExtractor& in);
	bool operator ==(const CTYNExtractor& in)const;
	bool operator !=(const CTYNExtractor& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual void GetSelection(short param, CSelectionDlgItemVector& items)const;
	virtual void SetSelection(short param, const CSelectionDlgItemVector& items);


	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}


	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	
protected:

	

	ERMsg ExecuteFromDaily(CFL::CCallback& callback=DEFAULT_CALLBACK);
	ERMsg ExecuteFromNormal(CFL::CCallback& callback=DEFAULT_CALLBACK);
	ERMsg ExecuteFromGrid(CFL::CCallback& callback=DEFAULT_CALLBACK);

	short m_extractType;
	short m_model;
	short m_sres;
	short m_period;
	CString m_TYNPath;
	CString m_inputFilePath;

	CString m_outputFilePath;
	bool m_bDeleteOldDB;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

	static const char* MODEL_NAME[NB_MODEL]; 
	static const char* SRES_NAME[NB_SRES]; 
	static const char* FILE_EXT[NB_EXT];
	static const char* PERIOD_NAME[NB_PERIOD];
	static const char* OBS_FILE_NAME;
	static CString GetFileName( short t);
	static CString GetFileName( short m, short s, short t);
};

