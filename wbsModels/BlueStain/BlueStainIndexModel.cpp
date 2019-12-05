//*********************************************************************
// 27/05/2019	1.2.3	Rémi Saint-Amant    Final version with non-limited
// 29/04/2018	1.2.2	Rémi Saint-Amant    Add limited model
// 23/03/2017	1.2.1	Rémi Saint-Amant    Bug corrected
// 20/09/2016	1.2.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 27/01/2015	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "BlueStainIndexModel.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{
	enum { O_BSI_FULL_ADD, O_BSI_FULL_MUL, O_BSI_LIMIT_ADD, O_BSI_LIMIT_MUL, NB_OUTPUT };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBlueStainIndexModel::CreateObject);



	const double CBlueStainIndexModel::PERCENTILS[NB_PROFILES][CBlueStainVariables::NB_VARIABLES][NB_LIMITS] =
	{//   lo       10%      90%     hi
		//Blue stain
		{//   lo       10%      90%     hi
			{-8.7   , -0.1  ,  5.2  ,  7.0	   }, //Minimum annual temperature(°C)
			{-2.3   ,  3.9  ,  9.6  , 11.7	   }, //Mean annual temperature(°C)
			{4.0    , 6.9   ,13.9   ,16.3	   }, //Maximum annual temperature(°C)
			{279.7  , 464.9 ,1410.1 ,1846.3	   }, //Total annual precipitation(mm)
			{11.2   , 13.0  , 19.1  , 20.8	   }, //Warmest quarter total precipitation(mm)
			{-21.1  ,  -8.0 ,   2.0 ,   3.8	   }, //Warmest quarter mean temperature(°C)
			{-2.7   ,  7.4  , 18.9  , 20.8	   }, //Coldest quarter total precipitation(mm)
			{-10.4  ,  -6.2 ,  10.7 ,  13.6	   }, //Coldest quarter mean temperature(°C)
			{41.8   ,118.6  ,314.7  ,480.5	   }, //Wettest quarter total precipitation(mm)
			{13.7   , 38.7  ,231.1  ,401.1	   }, //Wettest quarter mean temperature(°C)
			{78.6   ,120.7  ,337.7  ,462.8	   }, //Driest quarter total precipitation(mm)
			{12.6   , 21.6  ,139.1  ,202.0	   }, //Driest quarter mean temperature(°C)
			{-1343.5, -938.0,   -5.8,  187.9   }, //Annual aridity index(mm)
			{-421.3 ,-169.7 ,  108.2,  158.0   }, //Warmest quarter aridity index(mm)
			{-523.7 ,-291.7 ,  -42.5,    1.2   }, //Coldest quarter aridity index(mm)
			{-523.7 ,-398.2 ,   57.2,  128.2   }, //Wettest quarter aridity index(mm)
			{-277.0 ,-154.3 ,   42.2,  140.9   }, //Driest quarter aridity index(mm)
			{12.0   ,13.6   , 20.3  , 21.3	   }, //Warmest month mean temperature(°C)
			{-23.1  , -9.0  ,  -0.5 ,   2.1	   }, //Coldest month mean temperature(°C)
			{56.6   ,82.2   ,256.0  ,371.3     }, //Total precipitation in the wettest month(mm)
			{1.2    ,4.3    ,43.2   ,61.1      }, //Total precipitation in the driest month(mm)
			{596.0  ,848.7  ,1796.8 ,1979.8    }  //Degree day accumulation >5°C between 1 April and 31 August
		},
		//IPS typographus
		{
			{-9.54,-0.5,5.51,11.88},
			{-3.38,3.57,9.33,14.86},
			{2.79,7.49,13.47,17.9},
			{59.8,532,1205.4,2337.3},
			{9.92,13.43,18.25,22.34},
			{-20.87,-7.5,2.43,7.42},
			{-10.35,7.79,17.64,21.69},
			{-20.87,-5.24,12.92,19.94},
			{13.2,92.4,275.9,507.1},
			{3.6,44.3,169.3,355.5},
			{21,118.9,324.3,642.9},
			{3.6,31.8,115.6,216.6},
			{-1883.27,-727.71,-45.48,380.59},
			{-378.06,-173.48,108.45,249.42},
			{-574.54,-222.42,-52.87,33.46},
			{-790.35,-300.58,35.35,211.98},
			{-274.5,-127.4,81.66,249.42},
			{11.42,14.54,19.84,23.5},
			{-21.05,-10.15,0.42,6.53},
			{12.4,84.2,215.8,545.6},
			{0,3.6,31.8,77.9},
			{505.25,915.5,1583.7,2116.4}
		},
	};



	CLimits CBlueStainIndexModel::GetF(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], VariablesMask mask, size_t v, double f)
	{
		CLimits out;

		static const double LIMITS_POINT[2] = { 0.0, 100.0 };
		size_t ll = 0;
		for (size_t l = 0; l < NB_LIMITS; l++)
		{
			if (f >= LIMITS[v][l])
				ll = l + 1;
		}

		double F = 0;
		switch (ll)
		{
		case 0: F = LIMITS_POINT[0]; break;
		case 1: F = LIMITS_POINT[0] + (f - LIMITS[v][ll - 1]) / (LIMITS[v][ll] - LIMITS[v][ll - 1])*(LIMITS_POINT[1] - LIMITS_POINT[0]); break;
		case 2: F = LIMITS_POINT[1]; break;
		case 3: F = LIMITS_POINT[1] + (f - LIMITS[v][ll - 1]) / (LIMITS[v][ll] - LIMITS[v][ll - 1])*(LIMITS_POINT[0] - LIMITS_POINT[1]); break;
		case 4: F = LIMITS_POINT[0]; break;
		}


		ASSERT(F >= 0 && F <= 100);

		return CLimits(F / mask.count(), pow(F, 1.0 / mask.count()));
	}

	CLimits CBlueStainIndexModel::GetBSI(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], VariablesMask mask, const CModelStatVector& values)
	{
		CLimits BSI(0, 0);

		CTPeriod p = values.GetTPeriod();
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CLimits y;

			for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
			{
				if (mask[v])
				{
					CLimits f = GetF(LIMITS, mask, v, values[TRef][v]);
					y[ADD] += f[ADD];
					y[MUL] *= f[MUL];
				}
			}

			BSI += y / p.size();
		}


		return BSI;
	}


	CBlueStainIndexModel::CBlueStainIndexModel()
	{
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.2.4 (2019)";

	}

	CBlueStainIndexModel::~CBlueStainIndexModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CBlueStainIndexModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_profile = parameters[c].GetInt();

		return msg;
	}



	//This method is call to compute solution
	ERMsg CBlueStainIndexModel::OnExecuteAnnual()
	{
		ERMsg msg;

		VariablesMask full;
		full.set();
		array<VariablesMask,2> limited;
		limited[0].set(CBlueStainVariables::V_TMEAN);// Annual mean of daily mean temperature
		limited[0].set(CBlueStainVariables::V_PRCP);//Total annual precipitation
		limited[0].set(CBlueStainVariables::V_DRYQ_TMEAN);//Driest quarter mean temperature
		limited[0].set(CBlueStainVariables::V_DRYQ_AI);//Driest quarter water deficit
		limited[0].set(CBlueStainVariables::V_SUMMER_DD5);//Degree day accumulation >5°C between 1 April and 31 August

		limited[1].set(CBlueStainVariables::V_WARMQ_TMEAN);//Warmest quarter mean temperature (°C)
		limited[1].set(CBlueStainVariables::V_COLDQ_TMEAN);//Coldest quarter mean temperature (°C)
		limited[1].set(CBlueStainVariables::V_WARMQ_AI);//Warmest quarter water deficit (mm)
		limited[1].set(CBlueStainVariables::V_DRYQ_AI);// Driest quarter water deficit (mm)


		CBlueStainVariables blueStainVariables;
		CModelStatVector values;
		blueStainVariables.Execute(m_weather, values);
		CLimits BSIFull = GetBSI(PERCENTILS[m_profile], full, values);
		CLimits BSILimited = GetBSI(PERCENTILS[m_profile], limited[m_profile], values);

		m_output.Init(1, CTRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTM::ANNUAL, CTM::OVERALL_YEARS)), NB_OUTPUT, -9999);

		m_output[0][O_BSI_FULL_ADD] = BSIFull[ADD];
		m_output[0][O_BSI_FULL_MUL] = BSIFull[MUL];
		m_output[0][O_BSI_LIMIT_ADD] = BSILimited[ADD];
		m_output[0][O_BSI_LIMIT_MUL] = BSILimited[MUL];


		return msg;
	}




}

























