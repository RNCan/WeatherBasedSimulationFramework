#pragma once

#include <array>
#include <boost\dynamic_bitset.hpp>
#include <memory>

#include "netcdf"
#include "Basic/ApproximateNearestNeighbor.h"
#include "Simulation/AdvancedNormalStation.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/Mtrx.h"
#include "TaskBase.h"
#include "Basic\WeatherDefine.h"


namespace WBSF
{

	class CDailyDatabase;
	class CWeatherStation;
	class CMonthlyMeanGrid;



	typedef std::shared_ptr < netCDF::NcFile > NcFilePtr;
	typedef std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> CMonthlyVariables;
	typedef std::vector<CMonthlyVariables> CMonthlyVariableVector;
	//typedef std::vector< CMonthlyVariable > CMonthlyVariableVector;
	typedef boost::dynamic_bitset<size_t> CLandWaterBitset;


	class CHadGEM2_ES_MMGCreator : public CTaskBase
	{
	public:

		enum{ DIM_TIME, DIM_LEVEL, DIM_LAT, DIM_LON, NB_DIMS };
		enum TVariable{ V_TMIN, V_TMAX, V_PRCP, /*V_RELH,*/ V_SPEH, V_WNDS, NB_VARIABLES };

		typedef std::vector<float> COneVariableLayer;
		typedef std::array< std::array < COneVariableLayer, NB_VARIABLES>, 30> COneMonthData;

		//enum { NB_PERIOD_HISTORICAL=5, NB_FUTUR_PERIOD=11, NB_PERIOD_TYPE };
		//enum { HISTORICAL, FUTUR, NB_PERIOD_TYPE };
		enum TRCP{ RCP26, RCP45, RCP60, RCP85, NB_RCP };
		
		enum TAttributes { INPUT_PATH, OUTPUT_FILEPATH, NB_ATTRIBUTES };
		static const char* VARIABLES_NAMES[NB_VARIABLES];
		static const char* RCP_NAME[NB_RCP];
		
		//static std::string GetFileTitle(size_t RCP, size_t period, size_t var);
		//static size_t GetRPC(size_t periodType, size_t RPC);
		//static CTRef GetFirstTRef(size_t rcp, size_t periodNo);
		//static CTRef GetLastTRef(size_t rcp, size_t periodNo);
		//static CTPeriod GetPeriod(size_t rcp, size_t periodNo);
		static void ComputeMontlyStatistic(size_t i, const COneMonthData& data, CMonthlyVariables& montlhyStat);

		//std::string GetFilePath(size_t RCP, size_t period, size_t var)const;
		


		static std::string GetProjectionWKT();
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CHadGEM2_ES_MMGCreator); }

		CHadGEM2_ES_MMGCreator(void);
		~CHadGEM2_ES_MMGCreator(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:

		ERMsg ExportPoint(std::string filePath, int rcp, CGeoPoint pt, CCallback& callback);

		
		CLandWaterBitset m_landWaterMask;

		static CGeoExtents GetExtents();

		void GetDEM(CMatrix<float>& DEM);
		ERMsg CreateMMG(size_t rcp, CMonthlyMeanGrid& MMG, CCallback& callback);
		void GetMapOptions(CBaseOptions& options);
		void ConvertData(size_t v, std::vector<float>& data)const;
		ERMsg GetMMGForRCP(size_t rcp, CMonthlyVariableVector& dataOut, CCallback& callback);
		ERMsg GetLandWaterProfile(CLandWaterBitset& landWaterMask);
		ERMsg GetFileList(size_t rcp, std::vector<std::array<std::string, NB_VARIABLES>>& fileList)const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
//		static const int NB_PERIODS[NB_PERIOD_TYPE];

	};

	typedef std::array<NcFilePtr, CHadGEM2_ES_MMGCreator::NB_VARIABLES> NcFilePtrArray;
	//typedef std::vector<NcFilePtrArray> NcFilePtrVector;

}