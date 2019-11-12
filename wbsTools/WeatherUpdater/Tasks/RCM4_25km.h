#pragma once

#include <array>
#include "Basic/ApproximateNearestNeighbor.h"
//#include "ToolsBase.h"
#include "Simulation/AdvancedNormalStation.h"
#include "boost\dynamic_bitset.hpp"
#include "Geomatic/GDALBasic.h"
#include "Basic/Mtrx.h"
#include "Basic/WeatherDefine.h"

namespace WBSF
{
	class CDailyDatabase;
	class CDailyStation;



	typedef std::vector< std::array< std::vector<float>, NORMALS_DATA::NB_FIELDS> > CMonthlyVariableVector;
	typedef boost::dynamic_bitset<size_t> CLandWaterBitset;

	class CRCM4_ESM2_NAM_25km
	{
	public:

		enum TDimentsion { DIM_LATITUDE, DIM_LONGITUDE, DIM_TEMPORAL, DIM_VARIABLE, NM_DIM };
		enum TVariable { V_TMIN, V_TAIR, V_TMAX, V_PRCP, V_SPCH, V_WNDS, V_SRAD, V_PRES, NB_VARIABLES};

		enum TRCP { RCP45, RCP85, NB_RCP };
		static const char* VARIABLES_NAMES[NB_VARIABLES];
		static const HOURLY_DATA::TVarH VARIABLES[NB_VARIABLES];
		static const char* RCP_NAME[NB_RCP];
		//enum TVariable { V_TMIN, V_TAIR, V_TMAX, V_PRCP, V_SPCH, V_WIND_SPEED, V_SRAD, V_PRES, NB_VAR };
		//enum TATTRIBUTE { RCM_PATH, REGION, OUTPUT_FILEPATH, NB_ATTRIBUTE };
		//enum TATTRIBUTE_I { I_RCM_PATH = CToolsBase::I_NB_ATTRIBUTE, I_REGION, I_OUTPUT_FILEPATH, I_NB_ATTRIBUTE };

		CRCM4_ESM2_NAM_25km(void);
		~CRCM4_ESM2_NAM_25km(void);

		std::string m_path;
		std::string m_regionName;
		std::string m_outputFilePath;



		void Reset();
		//CRCM4_ESM2_22km_MMGCreator& operator =(const CRCM4_ESM2_22km_MMGCreator& in);
		//bool operator ==(const CRCM4_ESM2_22km_MMGCreator& in)const;
		//bool operator !=(const CRCM4_ESM2_22km_MMGCreator& in)const;
		//bool Compare(const CParameterBase& in)const;
		//CParameterBase& Assign(const CParameterBase& in);

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		//virtual void InitClass(const StringVector& option = EMPTY_OPTION);
		//virtual std::string GetClassID()const { return CLASS_NAME; }

		//virtual int GetNbAttribute()const { return I_NB_ATTRIBUTE; }
		//virtual std::string GetValue(size_t type)const;
		//virtual void SetValue(size_t type, const std::string& value);
		//ERMsg CreateMMG(CCallback& callback);

		CGeoExtents GetExtents();
		ERMsg GetMMGForPeriod(int periodType, int rcp, int periodNo, CMonthlyVariableVector& dataOut, CCallback& callback);
		void GetLandWaterProfile(CLandWaterBitset& landWaterMask);
		void ConvertData(size_t v, std::vector<float>& data)const;
		std::string GetPeriodName(int t, int rcp)const;
		std::vector<std::string> GetFileList(int periodType, int rcp, int periodNo)const;
		std::vector<std::string> GetFileList(std::string varName, int rcp)const;
		//int GetTotalNbDays(std::string varName)const;
		ERMsg ExportPoint(std::string filePath, int rcp, CGeoPoint pt, CCallback& callback);
		ERMsg ExportPoint(CWeatherStation& station, int rcp, CGeoPoint pt, CCallback& callback);


		void GetDEM(CMatrix<float>& DEM);
		static std::string GetPrjStr();

	protected:

		


		//static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
		//static const char* CLASS_NAME;

		CMatrix<float> m_DEM;
		CGeoExtents m_extents;
		CLandWaterBitset m_landWaterMask;
		std::string GetFilePath(std::string fileName);


		static float ConvertData(size_t v, float in);
	};

}