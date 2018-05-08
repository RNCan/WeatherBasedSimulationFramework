//*********************************************************************
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



	const double CBlueStainIndexModel::PERCENTILS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS] =
	{
		//  Min		  10%		  90%		  Max
		{ -8.58,   -0.16,    5.44,    7.02 },	  //V_TMIN_EXT
		{ -2.27,    3.84,    9.78,   11.67 },	  //V_TMEAN
		{ 4.05,    6.69,   13.86,   16.32 },	  //V_TMAX_EXT
		{ 279.70,  463.50, 1411.40, 1842.50 },	  //V_PRCP
		{ 11.11,   12.83,   19.07,   20.86 },	  //V_WARMQ_TMEAN
		{ -21.00,   -8.17,    2.13,    4.92 },	  //V_COLDQ_TMEAN
		{ -2.52,    6.37,   18.94,   20.86 },	  //V_WETQ_TMEAN
		{ -9.79,   -5.55,    7.27,   12.28 },	  //V_DRYQ_TMEAN
		{ 41.80,  117.70,  316.30,  480.50 },	  //V_WARMQ_PRCP
		{ 10.80,   37.70,  235.10,  401.00 },	  //V_COLDQ_PRCP
		{ 78.60,  120.40,  339.50,  461.90 },	  //V_WETQ_PRCP
		{ 10.80,   36.50,  127.70,  164.80 },	  //V_DRYQ_PRCP
		{ -1335.43, -945.13,  -20.68,  187.26 },  //V_AI
		{ -420.50, -175.90,  114.50,  148.87 },	  //V_WARMQ_AI
		{ -526.60, -294.40,  -42.50,   22.45 },	  //V_COLDQ_AI
		{ -526.60, -420.50,   53.93,  128.16 },	  //V_WETQ_AI
		{ -277.90, -148.50,   15.79,  140.60 },	  //V_DRYQ_AI
		{ 11.87,   13.38,   20.25,   21.35 },	  //V_WARMM_TMEAN
		{ -23.05,   -9.48,   -0.28,    2.12 },	  //V_COLDM_TMEAN
		{ 56.60,   81.00,  251.80,  371.30 },	  //V_WETM_PRCP
		{ 1.30,    4.20,   43.20,   61.10 },	  //V_DRYM_PRCP
		{ 593.90,  837.95, 1787.40, 1980.60 }     //V_SUMMER_DD5

		//Février 2018
		/*{-8.7   , -0.1  ,  5.2  ,  7.0	   },
		{-2.3   ,  3.9  ,  9.6  , 11.7	   },
		{4.0    , 6.9   ,13.9   ,16.3	   },
		{279.7  , 464.9 ,1410.1 ,1846.3	   },
		{11.2   , 13.0  , 19.1  , 20.8	   },
		{-21.1  ,  -8.0 ,   2.0 ,   3.8	   },
		{-2.7   ,  7.4  , 18.9  , 20.8	   },
		{-10.4  ,  -6.2 ,  10.7 ,  13.6	   },
		{41.8   ,118.6  ,314.7  ,480.5	   },
		{13.7   , 38.7  ,231.1  ,401.1	   },
		{78.6   ,120.7  ,337.7  ,462.8	   },
		{12.6   , 21.6  ,139.1  ,202.0	   },
		{-1343.5, -938.0,   -5.8,  187.9   },
		{-421.3 ,-169.7 ,  108.2,  158.0   },
		{-523.7 ,-291.7 ,  -42.5,    1.2   },
		{-523.7 ,-398.2 ,   57.2,  128.2   },
		{-277.0 ,-154.3 ,   42.2,  140.9   },
		{12.0   ,13.6   , 20.3  , 21.3	   },
		{-23.1  , -9.0  ,  -0.5 ,   2.1	   },
		{56.6   ,82.2   ,256.0  ,371.3     },
		{1.2    ,4.3    ,43.2   ,61.1      },
		{596.0  ,848.7  ,1796.8 ,1979.8    }*/
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
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.2.2 (2018)";

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

		return msg;
	}



	//This method is call to compute solution
	ERMsg CBlueStainIndexModel::OnExecuteAnnual()
	{
		ERMsg msg;

		VariablesMask full;
		full.set();
		VariablesMask limited;
		limited.set(CBlueStainVariables::V_TMEAN);
		limited.set(CBlueStainVariables::V_PRCP);
		limited.set(CBlueStainVariables::V_WARMQ_TMEAN);
		limited.set(CBlueStainVariables::V_DRYQ_AI);


		CBlueStainVariables blueStainVariables;
		CModelStatVector values;
		blueStainVariables.Execute(m_weather, values);
		CLimits BSIFull = GetBSI(PERCENTILS, full, values);
		CLimits BSILimited = GetBSI(PERCENTILS, limited, values);

		m_output.Init(1, CTRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTM::ANNUAL, CTM::OVERALL_YEARS)), NB_OUTPUT, -9999);

		m_output[0][O_BSI_FULL_ADD] = BSIFull[ADD];
		m_output[0][O_BSI_FULL_MUL] = BSIFull[MUL];
		m_output[0][O_BSI_LIMIT_ADD] = BSILimited[ADD];
		m_output[0][O_BSI_LIMIT_MUL] = BSILimited[MUL];


		return msg;
	}




}

























