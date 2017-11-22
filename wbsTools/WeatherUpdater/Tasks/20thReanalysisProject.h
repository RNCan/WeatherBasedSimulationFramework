#pragma once


#include "GeoRect.h"
#include "weatherDefine.h"
#include "_NETCDF\cxx\netcdfcpp.h"
#include "Statistic.h"
#include <boost\scoped_ptr.hpp>
#include "boost\multi_array.hpp"
#include "SCCallback.h"
//#include "MapBilFile.h"
#include "MonthlyMeanGrid.h"
#include "ToolsBase.h"

using namespace UtilWin;
typedef boost::multi_array<float, 3> CData3Array;
//typedef boost::multi_array<short, 2> CData2Array;
class CGDALDatasetEx;

class C20thReanalysisProject
{
public:
		
	enum TType {_2X2, GAUSS, NB_TYPE};
	C20thReanalysisProject(const CString& path="");
	~C20thReanalysisProject(void);

	void SetPath(const CString& path){	m_path = path;}

//	ERMsg CreateMMG(CString filePathIn, CString filePathOut, CSCCallBack& callback);
	ERMsg ExtractTopo(const CString& filePath, CSCCallBack& callback);
	ERMsg ExtractData(const CString& filePath, CSCCallBack& callback);
	ERMsg CreateDailyGrid( CString filePathIn, CString varName, const CString& filePathOut, CSCCallBack& callback);
	//ERMsg CreateCCGrid( CString varName, const CString& filePath, CSCCallBack& callback);

	static ERMsg GetGridInfo(const CString& filePath, int& sizeX, int& sizeY, int& nbBand, CGeoRectWP& rect, float& noData, double& cellSizeX, double& celleSizeY);


	int m_type;
private:

	static ERMsg ReadData( CString filePath, CString varName, CData3Array& data);
	static ERMsg ReadData( NcVar* pVarData, int x, int y, std::vector<float>& data);
	static ERMsg ReadData( CGDALDatasetEx& dataset, int x, int y, std::vector<float>& data);

	CString m_path;
};


class C20thRPTask : public CToolsBase
{
public:

	
	enum TATTRIBUTE {WORKING_DIR, NORMAL_DB, OUT_FILEPATH, TYPE, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_WORKING_DIR=CToolsBase::NB_ATTRIBUTE, I_NORMAL_DB, I_OUT_FILEPATH, I_TYPE, I_NB_ATTRIBUTE};


	C20thRPTask(void);
	virtual ~C20thRPTask(void);
	C20thRPTask(const C20thRPTask& in);

	void Reset();
	C20thRPTask& operator =(const C20thRPTask& in);
	bool operator ==(const C20thRPTask& in)const;
	bool operator !=(const C20thRPTask& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);
	

	virtual ERMsg Execute(CSCCallBack& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const CStringArray& option = EMPTY_OPTION);
	virtual CString GetClassID()const{return CLASS_NAME;}

	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual CString GetValue(short type)const;
	virtual void SetValue(short type, const CString& value);

	
protected:

	CString m_path;
	CString m_inputNormalFilePath;
	CString m_outputFilePath;
	int m_type;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

};

