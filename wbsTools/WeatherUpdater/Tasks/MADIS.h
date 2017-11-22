#pragma once

#include "GeoRect.h"
#include "weatherDefine.h"
#include "_NETCDF\cxx\netcdfcpp.h"
#include "Statistic.h"
#include <boost\scoped_ptr.hpp>
#include "boost\multi_array.hpp"
#include "SCCallback.h"

class CStation;
class CDailyData;
class CDailyYear;
class CDailyStation;

typedef boost::multi_array<float, 3> COneMonthGrid;
typedef boost::multi_array<CFL::CStatistic, 2> CStatisticMultiArray;

class CMADIS
{
public:
		
	CMADIS(const CString& path="");
	~CMADIS(void);

	void SetPath(const CString& path){	m_path = path;}

	//ERMsg GetDiffMap(const CString& filePath1, const CString& filePath2, const CString& filePath3, bool diff, CSCCallBack& callback);
	ERMsg ExtractLOC(const CString& filePath, CSCCallBack& callback);
	//ERMsg ExtractMask(const CString& filePath, CSCCallBack& callback);
	//ERMsg CreateCCGrid( short v, short firstccYear, short lastccYear, short m, const CString& filePath, CSCCallBack& callback);
	//ERMsg CreateCCGrid( short v, const CString& filePath, CSCCallBack& callback);
	//ERMsg CreateNormal( short firstccYear, short lastccYear, const CString& filePath, CSCCallBack& callback );
	//bool VerifyZero(short v);
	//ERMsg ExportData( short Year, short month, CGeoPointWP pt, const CString& filePath, CSCCallBack& callback );

	//static CPoint GetColRow( const CGeoPointWP& ptIn );
private:

	//ERMsg ReadData( short v, short year, short m, CRect rect, COneMonthGrid& data)const;
	//ERMsg ReadData( short v, short year, short m, CRect rect, CStatisticMultiArray& oneMonthStat)const;

	//static void DailyToStat(const COneMonthGrid& data, CStatisticMultiArray& oneMonthStat);
	//static double GetMonthlyRatio(const COneMonthGrid& dataRef, const COneMonthGrid& data);

	//CString GetFilePath(short v, short year, short m)const;
	CString m_path;


	
	//static CGeoGridWP GetDataGrid();
	//static CGeoGridWP DATA_GRID;
	//static const char* VAR_NAME[DAILY_DATA::NB_VAR_BASE];
};

