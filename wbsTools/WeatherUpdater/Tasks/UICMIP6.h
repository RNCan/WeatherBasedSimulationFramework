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
	//typedef std::vector<float> CLandWater;
	//typedef std::vector<float> COrog;
	typedef std::map<std::string, std::vector<std::string>> CMIP6FileList;
	
	//typedef std::array<NcFilePtr, CUICMIP6::NB_VARIABLES> NcFilePtrArray;



	class CUICMIP6 : public CTaskBase
	{
	public:

		enum { DIM_TIME, DIM_LEVEL, DIM_LAT, DIM_LON, NB_DIMS };
		enum TVariable { V_TMIN, V_TMAX, V_PRCP, V_SPEH, V_WNDS, NB_CMIP6_VARIABLES };

		typedef std::vector<float> COneVariableLayer;
		typedef std::vector< std::array < COneVariableLayer, NB_CMIP6_VARIABLES>> COneMonthData;

		
		enum TAttributes { WORKING_DIR, DOWNLOAD_DATA, CREATE_GRIBS, FIRST_YEAR, LAST_YEAR, FREQUENCY, MODEL, SSP, MIN_LAND_WATER, NB_ATTRIBUTES };
		static const char* VARIABLES_NAMES[NB_CMIP6_VARIABLES];
		static void ComputeMontlyStatistic(size_t i, size_t ii, const COneMonthData& data, CMonthlyVariables& montlhyStat);
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
		virtual bool IsDaily()const override { return true; }
		virtual bool IsMMG()const override { return true; }
		virtual bool IsGribs()const override { return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK)override;
		virtual ERMsg CreateMMG(std::string filePathOut, CCallback& callback)override;
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)override;

		virtual size_t GetNbAttributes()const { return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const { ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const { ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		//ERMsg ExportPoint(std::string filePath, int rcp, CGeoPoint pt, CCallback& callback);
		//CLandWaterMask m_landWaterMask;

		//static CGeoExtents GetExtents();

		//ERMsg GetDEM(CMatrix<float>& DEM);
		//ERMsg GetFileList(std::string model, std::string ssp, CMonthlyMeanGrid& MMG, CCallback& callback);
		
		void ConvertData(size_t v, std::vector<float>& data)const;
		ERMsg GetMMGForSSP(std::string model, std::string ssp, const CTPeriod& valid_period, const CGeoExtents& extents, const std::vector<float>& sftlf, float minLandWater, CMonthlyVariableVector& dataOut, CCallback& callback);
		//ERMsg GetMMGForSSP2(std::string model, std::string ssp, CGeoExtents extents, CLandWaterMask landWaterMask, float minLandWater, CMonthlyVariableVector& dataOut, CCallback& callback);
		
		ERMsg GetFileList(std::string model, std::string ssp, std::string ripf, std::string frequency, const CTPeriod& valid_period, CMIP6FileList& fileList)const;
		ERMsg save_sftlf(std::string sftlf_filepath, std::string new_sftlf_filepath);

		ERMsg CreateDailyGribs(CCallback& callback);
		ERMsg CreateMonthlyGribs(std::string filepath_out, std::string ripf, CCallback& callback);
		//ERMsg SaveInput(std::string sftlf_file_path, CLandWaterMask& landWaterMask);
		ERMsg save_orog(std::string orog_file_path, std::string new_orog_filepath, const std::vector<float>& sftlf, float minLandWater);
		
		ERMsg get_orog(std::string orog_filepath, std::vector<float>& data, float new_no_data);
		ERMsg load_geotif(std::string filepath, std::vector<float>& data);
		ERMsg get_sftlf(std::string sftlf_filepath, std::vector<float>& data) { return load_geotif(sftlf_filepath, data); }

		ERMsg load_orog_sftlf(std::vector<float>& orog, std::vector<float>& sftlf, float no_data_out, CCallback& callback);
		CBaseOptions GetMapOption()const;
		ERMsg GetMapOptions(std::string orog_file_path, CBaseOptions& options)const;

		ERMsg GetMonthlyData(const CMIP6FileList& fileList, const CTPeriod& valid_period, const CGeoExtents& extents, const std::vector<float>& sftlf, float minLandWater, float no_data_out, COneMonthData& data, CCallback& callback);


		static CTPeriod get_period(const std::string& period);
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		

	};

	typedef std::array<NcFilePtr, CUICMIP6::NB_CMIP6_VARIABLES> NcFilePtrArray;

}