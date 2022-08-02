#pragma once

//#include <array>
//#include "boost\dynamic_bitset.hpp"
//#include <memory>
#include "netcdf"
//#include "ApproximateNearestNeighbor.h"
//#include "ToolsBase.h"
#include "Simulation/AdvancedNormalStation.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/Mtrx.h"


namespace WBSF
{


	class CDailyDatabase;
	class CDailyStation;
	class CMonthlyMeanGrid;



	typedef std::shared_ptr < netCDF::NcFile > NcFilePtr;
	typedef std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> CMonthlyVariable;
	typedef std::vector< CMonthlyVariable > CMonthlyVariableVector;


	class CCMIP6_MMGCreator 
	{
	public:

		enum { DIM_TIME, DIM_LAT, DIM_LON, NB_DIMS };
		enum TVariablesIn { I_TMIN, I_TMAX, I_PRCP, I_SPEH, I_WNDS, NB_VARIABLES_IN };
		enum TVariablesOut { O_TMIN_MN, O_TMAX_MN, O_TMIN_SD, O_TMAX_SD, O_PRCP_TT, O_SPEH_MN, O_RELH_MN, O_RELH_SD, O_WNDS_MN, O_WNDS_SD, NB_VARIABLES_OUT };
		enum { HISTORICAL, FUTURE, NB_PERIOD_TYPE };
		

		static const char* VARIABLES_NAMES[NB_VARIABLES_IN];
		


		std::string GetFileName(int period, int RCP, int var);
		//static std::string GetProjectionWKT();
		//static CGeoExtents GetGCM4Extents();

		typedef std::array<NcFilePtr, NB_VARIABLES_IN> NcFilePtrArray;
		typedef std::array< std::vector<float>, NB_VARIABLES_IN> CDataStructIn;
		

		std::string m_path;
		std::string m_output_filepath;
		std::string m_model_name;
		std::string m_ssp_name;

		CCMIP6_MMGCreator(void);
		~CCMIP6_MMGCreator(void);

		void Reset();
		
		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		//ERMsg CreateMMG(CCallback& callback);
		ERMsg CreateMMG(int rcp, CMonthlyMeanGrid& MMG, CCallback& callback);

		void ConvertData(CDataStructIn& dataIn)const;
		void GetOptions(CBaseOptions& options);
		ERMsg GetDEM(const std::string& file_path, CMatrix<float>& DEM);
		ERMsg GetLAF(const std::string& file_path, CMatrix<float>& LAF); //Land area fraction

	protected:


		CMatrix<float> m_DEM;
		CMatrix<float> m_LAF;

		//GeoBasic::CGeoExtents m_extents;
		//CLandWaterBitset m_landWaterMask;
		std::string GetFilePath(std::string fileName);

	};

}