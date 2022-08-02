#pragma once

#include <array>
#include <boost\dynamic_bitset.hpp>
#include <memory>
#include <netcdf>


#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/Mtrx.h"
#include "Basic\WeatherDefine.h"
#include "Geomatic/GDALBasic.h"
#include "Simulation/AdvancedNormalStation.h"
#include "TaskBase.h"



namespace WBSF
{

	class CDailyDatabase;
	class CWeatherStation;
	class CMonthlyMeanGrid;


	typedef std::shared_ptr < netCDF::NcFile > NcFilePtr;
	typedef std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> CMonthlyVariables;
	typedef std::vector<CMonthlyVariables> CMonthlyVariableVector;
	typedef boost::dynamic_bitset<size_t> CLandWaterBitset;
	typedef std::map<std::string, std::vector<std::string>> CMIP6FileList;
	
	//typedef std::array<NcFilePtr, CUICMIP6::NB_VARIABLES> NcFilePtrArray;



	class CUICMIP6 : public CTaskBase
	{
	public:

		enum { DIM_TIME, DIM_LEVEL, DIM_LAT, DIM_LON, NB_DIMS };
		enum TVariable { V_TMIN, V_TMAX, V_PRCP, V_SPEH, V_WNDS, NB_CMIP6_VARIABLES };

		typedef std::vector<float> COneVariableLayer;
		typedef std::array< std::array < COneVariableLayer, NB_CMIP6_VARIABLES>, 30> COneMonthData;

		//enum TRCP { RCP26, RCP45, RCP60, RCP85, NB_RCP };
		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, MODEL, NB_ATTRIBUTES };
		static const char* VARIABLES_NAMES[NB_CMIP6_VARIABLES];
		//static const char* RCP_NAME[NB_RCP];
		static void ComputeMontlyStatistic(size_t i, const COneMonthData& data, CMonthlyVariables& montlhyStat);
		static size_t GetVar(std::string name);

		static std::string GetProjectionWKT();
		static const char* CLASS_NAME();
		static CTaskPtr create() { return CTaskPtr(new CUICMIP6); }

		CUICMIP6(void);
		~CUICMIP6(void);


		virtual const char* ClassName()const { return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const { return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const { return DESCRIPTION_TITLE_ID; }
		virtual bool IsDaily()const { return true; }
		virtual bool IsMMG()const { return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg CreateMMG(std::string filePathOut, CCallback& callback);


		virtual size_t GetNbAttributes()const { return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const { ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const { ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		//ERMsg ExportPoint(std::string filePath, int rcp, CGeoPoint pt, CCallback& callback);
		CLandWaterBitset m_landWaterMask;

		//static CGeoExtents GetExtents();

		//ERMsg GetDEM(CMatrix<float>& DEM);
		ERMsg GetFileList(std::string model, std::string ssp, CMonthlyMeanGrid& MMG, CCallback& callback);
		ERMsg GetMapOptions(std::string orog_file_path, CBaseOptions& options);
		void ConvertData(size_t v, std::vector<float>& data)const;
		ERMsg GetMMGForSSP(std::string model, std::string ssp, CGeoExtents extents , CMonthlyVariableVector& dataOut, CCallback& callback);
		ERMsg GetLandWaterProfile(std::string sftlf_file_path, CLandWaterBitset& landWaterMask);
		ERMsg GetFileList(std::string model, std::string ssp, CMIP6FileList& fileList)const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* MODELS_NAME;

	};

	typedef std::array<NcFilePtr, CUICMIP6::NB_CMIP6_VARIABLES> NcFilePtrArray;

}