#pragma once

#include "TaskBase.h"
#include "basic/weatherStation.h"
#include "basic/FileStamp.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "Geomatic/GridInterpolBase.h"
#include "Geomatic/Variogram.h"

namespace WBSF
{
	
	class COptimizeInfo
	{
	public:
		
		size_t m_y;
		//std::array<size_t, 2> m_ij;
		CGridInterpolParam m_param;
		CXValidationVector m_XValidation;
		std::vector<double> m_error;
		std::vector<float> m_TITAN_Score;
		//CStatisticXY m_stat;
		CVariogram m_variogram;
	};

	typedef std::vector<COptimizeInfo> COptimizeInfoVector;

	class CWeatherGenerator;

	//**************************************************************
	//static const size_t NB_QC_VAR = 4;
	//static const size_t NB_QC_VAR = 4;
#define NB_QC_VAR 5
	
	typedef std::pair<CGridPointVectorPtr, std::vector<std::array<size_t, 2>>> CQCPointInfo;
	typedef  std::array < std::array<std::vector <CQCPointInfo>, NB_QC_VAR>, 12> CQCPointData;

	class CQualityControl
	{
	public:

		enum TOutput{ OT_HOURLY, OT_DAILY, NB_OUTPUT_TYPES };
		enum TATTRIBUTE { HRDPS_PATH, HRRR_PATH, OUTPUT_PATH, UPDATE_LAST_N_DAYS, NB_ATTRIBUTES };


		CQualityControl(void);
		virtual ~CQualityControl(void);


		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		ERMsg ExecuteHourly(CCallback& callback);
		ERMsg ExecuteDaily(CCallback& callback);
		ERMsg CheckDailyWithNormal(CCallback& callback);
		ERMsg InitDefaultWG(CWeatherGenerator& WG, CCallback& callback);
		ERMsg CheckBasicValue(CCallback& callback);
		
		ERMsg CreateValidation(CCallback& callback);
		ERMsg ApplyValidation(CCallback& callback);
		ERMsg CreateValidation2(CCallback& callback);
		


		ERMsg CreateVariogram(CCallback& callback);
		static ERMsg OptimizeParameter(size_t v, const CQCPointInfo& pts, COptimizeInfo& p, CCallback& callback);
		static CGridInterpolParamVector GetParamterset(size_t v);

		static ERMsg LoadStations(const StringVector& file_path, std::vector<CLocationVector>& stations, CCallback& callback);
		static ERMsg LoadStations(const StringVector& file_path, std::vector<CLocationVector>& stations, CQCPointData& pPts, std::set<int> years, CCallback& callback);
		static ERMsg LoadStations(const StringVector& file_path, std::vector<CWeatherStationVector>& stations, int year, CCallback& callback);
		static ERMsg LoadPts(std::vector<CWeatherStationVector>& stations, CQCPointInfo& pPts, CTRef TRef, HOURLY_DATA::TVarH v, CCallback& callback);

		static bool IsBasicCheckValid(size_t v, float value);
		static bool IsNormalsCheckValid(size_t v, float value, const std::array<CStatistic, HOURLY_DATA::NB_VAR_H>& normals_stats);
	};

}