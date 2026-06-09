#pragma once

#include <array>
#include <boost\dynamic_bitset.hpp>
#include <memory>
#include <netcdf>


#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/Mtrx.h"
#include "Basic/WeatherDefine.h"
#include "Basic/AdvancedNormalStation.h"
#include "Geomatic/GDALBasic.h"

#include "TaskBase.h"



namespace WBSF
{

	class CDailyDatabase;
	class CWeatherStation;
	class CMonthlyMeanGrid;


	typedef std::shared_ptr < netCDF::NcFile > NcFilePtr;
	typedef std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> CMonthlyVariables;
	typedef std::vector<CMonthlyVariables> CMonthlyVariableVector;
	typedef std::vector<std::string> CMIP6FileList;


	enum TVersion { VERSION_1_0, VERSION_1_1, VERSION_1_2, VERSION_2_0, NB_VERSIONS };
	static const std::array<std::string, NB_VERSIONS> INDEX_NAMES = { "index_md5.txt","index_v1.1_md5.txt","index_v1.2_md5.txt","index_v2.0_md5.txt" };

	class CNEX_GDDP_CMIP6
	{
	public:

		enum TInfo { I_SOURCE, I_MODEL, I_SPP, I_RUN, I_VARIABLE, I_FILE_NAME, NB_INFO };

		CNEX_GDDP_CMIP6(const std::string& URL = "")
		{
			parse(URL);
		}

		void parse(const std::string& URL)
		{
			m_info.clear();
			if (!URL.empty())
			{
				m_info = WBSF::Tokenize(URL, "/");
				assert(m_info.size() == NB_INFO);
			}
		}

		std::string GetID()const 
		{ 
			assert(m_info.size() == NB_INFO);  
			return m_info[I_MODEL] + "_" + m_info[I_SPP] + "_" + m_info[I_VARIABLE] + "_" + to_string(GetYear());
		}

		bool is_good_model(const std::string& model)const { assert(m_info.size() == NB_INFO);  return m_info[I_MODEL] == model; }
		bool is_good_spp(const std::string& spp)const {	assert(m_info.size() == NB_INFO);return m_info[I_SPP] == spp;}
		bool is_valid_year(int first_year, int last_year)const { assert(m_info.size() == NB_INFO);  return GetYear() >= first_year && GetYear() <= last_year; }

		//string GetPath()const {}

		int GetYear()const
		{
			//hurs_day_ACCESS-CM2_historical_r1i1p1f1_gn_1950.nc
			std::vector<std::string> element = WBSF::Tokenize(m_info[I_FILE_NAME], "_");
			assert(element.size() == 7 || element.size() == 8);

			return stoi(element[6]);
		}

		std::string GetURL()const
		{
			std::string URL;
			for (size_t i = 0; i < m_info.size(); i++)
			{
				if (i > 0)
					URL += "/";
				URL += m_info[i];
			}
			return URL;
		}


		std::vector<std::string> m_info;

	};




	class CUICMIP6 : public CTaskBase
	{
	public:

		enum { DIM_TIME, DIM_LEVEL, DIM_LAT, DIM_LON, NB_DIMS };
		enum TVariable { V_TMIN, V_TMAX, V_PRCP, V_SPEH, V_WNDS, NB_CMIP6_VARIABLES };//V_SRAD, 

		typedef std::vector<float> COneVariableLayer;
		typedef std::vector< std::array < COneVariableLayer, NB_CMIP6_VARIABLES>> COneMonthData;


		enum TAttributes { WORKING_DIR, DOWNLOAD_DATA, CREATE_GRIBS, FIRST_YEAR, LAST_YEAR, GEO_DOMAIN, MODEL, SSP, SHOW_CURL, NB_ATTRIBUTES };
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


		ERMsg SaveData(size_t y, int year, std::array< CGDALDatasetEx, NORMALS_DATA::NB_FIELDS>& grid, CMonthlyVariableVector& data, CCallback& callback);
		ERMsg Download(CCallback& callback);
		ERMsg DownloadFix(std::string prefix, CCallback& callback);
		ERMsg DownloadData(CCallback& callback);

		//From NASA
		ERMsg DownloadFilesIndex(CCallback& callback);
		ERMsg GetFilesIndex(std::vector<CNEX_GDDP_CMIP6>& index, CCallback& callback);
		ERMsg DownloadDataNASA(CCallback& callback);


		void ConvertData(size_t v, std::vector<float>& data)const;
		ERMsg GetMMGForSSP(std::string model, std::string ssp, int year, const CGeoExtents& extents, CMonthlyVariableVector& dataOut, CCallback& callback);
		//ERMsg GetMMGForSSP2(std::string model, std::string ssp, CGeoExtents extents, CLandWaterMask landWaterMask, float minLandWater, CMonthlyVariableVector& dataOut, CCallback& callback);

		ERMsg GetFileList(std::string model, std::string ssp, int year, CMIP6FileList& fileList)const;
		//ERMsg save_sftlf(std::string sftlf_filepath, std::string new_sftlf_filepath);

		ERMsg CreateDailyGribs(CCallback& callback);
		//ERMsg CreateMonthlyGribs(std::string filepath_out, std::string ripf, CCallback& callback);
		//ERMsg SaveInput(std::string sftlf_file_path, CLandWaterMask& landWaterMask);
		ERMsg save_orog(std::string orog_file_path, std::string new_orog_filepath);

		ERMsg get_orog(std::string orog_filepath, std::vector<float>& data, float new_no_data);
		ERMsg load_geotif(std::string filepath, std::vector<float>& data);
		//ERMsg get_sftlf(std::string sftlf_filepath, std::vector<float>& data) { return load_geotif(sftlf_filepath, data); }

		//ERMsg load_orog_sftlf(std::vector<float>& orog, std::vector<float>& sftlf, float no_data_out, CCallback& callback);
		//CBaseOptions GetMapOption()const;
		ERMsg GetMapOptions(std::string orog_file_path, CBaseOptions& options)const;

		//ERMsg GetMonthlyData(const CMIP6FileList& fileList, const CTPeriod& valid_period, const CGeoExtents& extents, const std::vector<float>& sftlf, float minLandWater, float no_data_out, COneMonthData& data, CCallback& callback);

		bool m_bWarningFixed30DaysData;
		bool m_bWarningMissingFeb29;

		static CTPeriod get_period(int year1, int year2);
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;


	};

	typedef std::array<NcFilePtr, CUICMIP6::NB_CMIP6_VARIABLES> NcFilePtrArray;

}