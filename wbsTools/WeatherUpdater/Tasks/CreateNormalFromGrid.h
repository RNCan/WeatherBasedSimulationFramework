#pragma once

#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
#include "2DimArray.h"


class CNormalsStation;
class CNormalsDatabase;

class CCreateNormalFromGrid : public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT_FILEPATH, OUTPUT_FILEPATH, FILTER_FILEPATH, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT_FILEPATH=CToolsBase::NB_ATTRIBUTE, I_OUTPUT_FILEPATH, I_FILTER_FILEPATH, I_NB_ATTRIBUTE};


	CCreateNormalFromGrid(void);
	virtual ~CCreateNormalFromGrid(void);
	CCreateNormalFromGrid(const CCreateNormalFromGrid& in);

	void Reset();
	CCreateNormalFromGrid& operator =(const CCreateNormalFromGrid& in);
	bool operator ==(const CCreateNormalFromGrid& in)const;
	bool operator !=(const CCreateNormalFromGrid& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}

	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	
protected:

	
	//bool FillStation(CNormalsStation& station, CArray<float>& data, CNormalsDatabase& inputDB);
//	ERMsg CreateDatabaseNormal(int i, CFL::CCallback& callback);
	
	CString m_inputFilePath;
	CString m_outputFilePath;
	CString m_filterFilePath;
	CGeoRect m_boundingBox;

	//int m_maxDistance;


	//CStdioFile m_tempFile;
	//CStdioFile m_tempFile2;

	//static void LoadData(CString filePathT, CString filePathP, CLocArray& loc, CFloatMatrix& data);
	
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

	static ERMsg CreateDatabase(int cc, int p, CNormalsDatabase& inputDB, CNormalsDatabase& outputDB, CFL::CCallback& callback);
};

