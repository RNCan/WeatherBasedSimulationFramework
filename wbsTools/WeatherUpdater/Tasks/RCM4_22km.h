#pragma once

#include <array>
#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
#include "AdvancedNormalStation.h"
#include "boost\dynamic_bitset.hpp"
#include "GDALBasic.h"
#include "Mtrx.h"

class CDailyDatabase;
class CDailyStation;



typedef std::vector< std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> > CMonthlyVariableVector;
typedef boost::dynamic_bitset<size_t> CLandWaterBitset;

class CRCM4_ESM2_22km_MMGCreator : public CToolsBase
{
public:

	enum TDimentsion { DIM_LATITUDE, DIM_LONGITUDE, DIM_TEMPORAL, DIM_VARIABLE, NM_DIM };
	enum TVarable{ V_TMIN, V_TMAX, V_PRCP, V_SP_HUM, V_WIND, NB_VARIABLES };

	enum TRCP{ RCP45, RCP85, NB_RCP };
	static const char* VARIABLES_NAMES[NB_VARIABLES];
	static const char* RCP_NAME[NB_RCP];
	enum TVariable { TMIN_AR5, TMAX_AR5, PRCP_AR5, VAPPRES_AR5, WIND_SPEED_AR5, NB_VAR};
	enum TATTRIBUTE {RCM_PATH, REGION, OUTPUT_FILEPATH, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_RCM_PATH=CToolsBase::I_NB_ATTRIBUTE, I_REGION, I_OUTPUT_FILEPATH,I_NB_ATTRIBUTE};

	CRCM4_ESM2_22km_MMGCreator(void);
	~CRCM4_ESM2_22km_MMGCreator(void);

	void Reset();
	CRCM4_ESM2_22km_MMGCreator& operator =(const CRCM4_ESM2_22km_MMGCreator& in);
	bool operator ==(const CRCM4_ESM2_22km_MMGCreator& in)const;
	bool operator !=(const CRCM4_ESM2_22km_MMGCreator& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const StringVector& option = EMPTY_OPTION);
	virtual std::string GetClassID()const{return CLASS_NAME;}

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);
	//ERMsg CreateMMG(CFL::CCallback& callback);

	GeoBasic::CGeoExtents GetExtents();
	ERMsg GetMMGForPeriod(int periodType, int rcp, int periodNo, CMonthlyVariableVector& dataOut, CFL::CCallback& callback);
	void GetLandWaterProfile(CLandWaterBitset& landWaterMask);
	void ConvertData(size_t v, std::vector<float>& data)const;
	std::string GetPeriodName(int t, int rcp)const;
	std::vector<std::string> GetFileList(int periodType, int rcp, int periodNo)const;
	std::vector<std::string> GetFileList(std::string varName, int rcp)const;
	int GetTotalNbDays(std::string varName)const;
	ERMsg ExportPoint(std::string filePath, int rcp, GeoBasic::CGeoPoint pt, CFL::CCallback& callback);

	void GetDEM(CFL::CMatrix<float>& DEM);
	static std::string GetPrjStr();

protected:

	std::string m_path;
	std::string m_regionName;
	std::string m_outputFilePath;
	
	
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

	CFL::CMatrix<float> m_DEM;
	GeoBasic::CGeoExtents m_extents;
	CLandWaterBitset m_landWaterMask;
	std::string GetFilePath( std::string fileName);
	
};

