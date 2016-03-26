#pragma once

#include <array>
#include "boost\dynamic_bitset.hpp"
#include <memory>
#include "netcdf"
#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
#include "AdvancedNormalStation.h"
#include "GDALBasic.h"
#include "Mtrx.h"

class CDailyDatabase;
class CDailyStation;
class CMonthlyMeanGrid;



typedef std::auto_ptr < netCDF::NcFile > NcFilePtr;
typedef std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> CMonthlyVariable;
typedef std::vector< CMonthlyVariable > CMonthlyVariableVector;


class CGCM4_ESM2_10km_MMGCreator : public CToolsBase
{
public:

	enum{ DIM_TIME, DIM_LEVEL, DIM_LAT, DIM_LON, NB_DIMS };
	enum TVariable{ V_TMIN, V_TMAX, V_PRCP, V_VAPOR, V_WIND, NB_VARIABLES };// V_SRAD, 
	enum { HISTORICAL, FUTURE, NB_PERIOD_TYPE };
	enum TRCP{ RCP26, RCP45, RCP85, NB_RCP };
	static const char* VARIABLES_NAMES[NB_VARIABLES];
	static const char* RCP_NAME[NB_RCP];
	static std::string GetFileName(int period, int RCP, int var);
	static std::string GetProjectionWKT();
	static GeoBasic::CGeoExtents GetGCM10Extents();

	typedef std::array<NcFilePtr, NB_VARIABLES> NcFilePtrArray;

	enum TATTRIBUTE {RCM_PATH, OUTPUT_FILEPATH,NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_RCM_PATH=CToolsBase::I_NB_ATTRIBUTE, I_OUTPUT_FILEPATH, I_NB_ATTRIBUTE};

	
	std::string m_path;
	std::string m_outputFilePath;



	CGCM4_ESM2_10km_MMGCreator(void);
	~CGCM4_ESM2_10km_MMGCreator(void);

	void Reset();
	CGCM4_ESM2_10km_MMGCreator& operator =(const CGCM4_ESM2_10km_MMGCreator& in);
	bool operator ==(const CGCM4_ESM2_10km_MMGCreator& in)const;
	bool operator !=(const CGCM4_ESM2_10km_MMGCreator& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const StringVector& option = EMPTY_OPTION);
	virtual std::string GetClassID()const{return CLASS_NAME;}

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);
	
	ERMsg CreateMMG(int rcp, CMonthlyMeanGrid& MMG, CFL::CCallback& callback);
	void ConvertData(CMonthlyVariable& data)const;
	void GetOptions(CBaseOptions& options);
	void GetDEM(CFL::CMatrix<float>& DEM);

protected:

	
	CFL::CMatrix<float> m_DEM;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

	//GeoBasic::CGeoExtents m_extents;
	//CLandWaterBitset m_landWaterMask;
	std::string GetFilePath( std::string fileName);
	
};

