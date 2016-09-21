//*********************************************************************
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
	enum { O_BSI_FULL_ADD, O_BSI_FULL_MUL, NB_OUTPUT };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBlueStainIndexModel::CreateObject);


	
	const double CBlueStainIndexModel::PERCENTILS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS] =
	{
		//  Min		  10%		  90%		  Max
		{-8.6, -0.3, 5.2, 7.3},
		{ -2.3, 3.7, 9.9, 11.7 },
		{ 3.8, 8.3, 14.8, 17.0 },
		{ 128.3, 528.3, 1389.4, 1816.4 },
		{ 11.8, 13.7, 19.1, 20.9 },
		{ -23.8, -7.3, 2.2, 4.0 },
		{ -4.4, 2.7, 18.2, 20.9 },
		{ -23.8, -6.1, 11.0, 18.3 },
		{ 42.3, 105.0, 343.5, 597.4 },
		{ 9.4, 34.5, 216.7, 490.2 },
		{ 37.8, 130.2, 349.7, 491.4 },
		{ 9.4, 33.7, 134.8, 218.8 },
		{ -1316.8, -929.1, -48.6, 338.1 },
		{ -460.5, -167.6, 97.5, 187.6 },
		{ -606.6, -340.3, -51.1, 8.9 },
		{ -606.6, -351.4, 39.3, 161.2 },
		{ -266.9, -135.7, 37.8, 184.0 },
		{ 12.6, 14.7, 20.7, 22.4 },
		{ -25.0, -10.2, 0.2, 2.5 },
		{ 22.0, 88.1, 260.6, 431.1 },
		{ 0.4, 3.7, 45.8, 63.6 },
		{ 647.9, 960.6, 1790.2, 1983.5 },
	};
	 

	CLimits CBlueStainIndexModel::GetF(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], size_t v, size_t nbVars, double f)
	{
		CLimits out;

		static const double LIMITS_POINT[2] = { 0.0, 100.0 };
		size_t ll = 0;
		for (size_t l = 0; l < NB_LIMITS; l++)
		{
			if (f >= LIMITS[v][l])
				ll = l+1;
		}
		
		double F = 0;
		switch(ll)
		{
		case 0: F = LIMITS_POINT[0]; break;
		case 1: F = LIMITS_POINT[0] + (f - LIMITS[v][ll-1]) / (LIMITS[v][ll] - LIMITS[v][ll-1])*(LIMITS_POINT[1] - LIMITS_POINT[0]); break;
		case 2: F = LIMITS_POINT[1]; break;
		case 3: F = LIMITS_POINT[1] + (f - LIMITS[v][ll-1]) / (LIMITS[v][ll] - LIMITS[v][ll-1])*(LIMITS_POINT[0] - LIMITS_POINT[1]); break;
		case 4: F = LIMITS_POINT[0]; break;
		}

		
		ASSERT(F>=0 && F<=100);
		//return CLimits(F / nbVars, pow(max(1.0, F), 1.0 / nbVars));
		return CLimits(F / nbVars, pow(F, 1.0 / nbVars));
	}

	CLimits CBlueStainIndexModel::GetBSI(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], const CSelectionVars& vars, const CModelStatVector& values)
	{

		CLimits BSI(0, 0);

		CTPeriod p = values.GetTPeriod();
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CLimits y;

			for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
			{
				if (vars[v])
				{
					CLimits f = GetF(LIMITS, v, vars.count(), values[TRef][v]);
					y[ADD] += f[ADD];
					y[MUL] *= f[MUL];
				}
			}

			BSI += y/p.size();
		}


		return BSI;
	}


	CBlueStainIndexModel::CBlueStainIndexModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 3;
		VERSION = "1.2.0 (2016)";

		double m_lo = 10;
		double m_hi = 90;
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
		m_vars = CSelectionVars(parameters[c++].GetSizeT() );
		m_lo = parameters[c++].GetReal();
		m_hi = parameters[c++].GetReal();
		

		return msg;
	}


	
	//This method is call to compute solution
	ERMsg CBlueStainIndexModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CBlueStainVariables blueStainVariables;
		CModelStatVector values;
		blueStainVariables.Execute(m_weather, values);
		CLimits BSIFull = GetBSI(PERCENTILS, m_vars, values);

		m_output.Init(1, CTRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTM::ANNUAL, CTM::OVERALL_YEARS)), NB_OUTPUT, -9999);

		m_output[0][O_BSI_FULL_ADD] = BSIFull[ADD];
		m_output[0][O_BSI_FULL_MUL] = BSIFull[MUL];

		CSelectionVars test(5);
		ASSERT(test.to_ullong() == 5);


		return msg;
	}





	//*****************************************************************************************************************
	//Next methods are for Parameterization

	void CBlueStainIndexModel::AddAnnualResult(const StringVector& header, const StringVector& data)
	{
		enum TPupasion{ I_NAME, I_ID, I_COUNTRY, I_YEAR, I_LATITUDE, I_LONGITUDE, I_ELEVATION, I_CERATOCYSTIS_POLONICA, I_IPS_TYPOGRAPHUS, I_KEY_YEAR, I_FIRST_VAR, I_LAST_VAR = I_FIRST_VAR+22, NB_INPUTS };

		int year = ToInt(data[I_YEAR]);

		/*for (auto it = m_weather.begin(); it != m_weather.end();)
		{
			if (it->first == year)
				it++;
			else
				it = m_weather.erase(it);
		}*/
			
		//ASSERT(m_weather.GetEntireTPeriod().size() == 1);

		//CBlueStainVariables blueStainVariables;
		//CModelStatVector values;
		//blueStainVariables.Execute(m_weather, values);


		std::vector<double> obs(CBlueStainVariables::NB_VARIABLES + 1);


		for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
			obs[v] += ToDouble(data[I_FIRST_VAR + v]);

		obs[CBlueStainVariables::NB_VARIABLES] = ToDouble(data[I_CERATOCYSTIS_POLONICA]);


		CTRef TRef(year);
		m_SAResult.push_back(CSAResult(TRef, obs));
	}

	void CBlueStainIndexModel::GetFValueAnnual (CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//now compare simuation with observation data
			double percentils[CBlueStainVariables::NB_VARIABLES][NB_LIMITS] = { 0 };
			for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
			{

				CStatisticEx stat;
				for (size_t i = 0; i < m_SAResult.size(); i++)
					if (m_SAResult[i].m_obs[CBlueStainVariables::NB_VARIABLES]==1)
						stat += m_SAResult[i].m_obs[v];

				//percentils[v][L_LO] = stat[Qᴸ];
				//percentils[v][L_25] = stat[Q¹];//stat.percentil(m_lo);
				//percentils[v][L_75] = stat[Q³];//stat.percentil(m_hi);
				//percentils[v][L_HI] = stat[Qᴴ];

				percentils[v][L_LO] = stat[LOWEST];
				percentils[v][L_10] = stat.percentil(10);
				percentils[v][L_90] = stat.percentil(90);
				percentils[v][L_HI] = stat[HIGHEST];
			}


			ofStream file;
			file.open("D:/testVars1_all2.csv");
			file << "K,N,BSI"<< endl;

			size_t nbVars = 4194304;
			for (size_t ii = 1; ii <nbVars; ii++)
			{
				//execute model with actual parameters
				m_vars = CSelectionVars(ii);

				//if (m_vars.count() >= 5 && m_vars.count() <= 10)
				{




					CStatistic test;
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (m_SAResult[i].m_obs[CBlueStainVariables::NB_VARIABLES]==1)
						{
							CModelStatVector inputs(1, m_SAResult[i].m_ref, CBlueStainVariables::NB_VARIABLES, -999);
							for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
								inputs[0][v] = m_SAResult[i].m_obs[v];

							CLimits BSIFull = GetBSI(percentils, m_vars, inputs);

							double obsV = m_SAResult[i].m_obs[CBlueStainVariables::NB_VARIABLES] * 100;
							double simV = BSIFull[MUL];
							test += simV;

							stat.Add(simV, obsV);
						}
					}

					file << to_string(ii) << "," << m_vars.count() << "," << test[MEAN] << endl;
				}
			}


			file.close();
			/*else
			{
				stat.Add(999, 0);
				stat.Add(-999, 999);
				stat.Add(0, 100);
				stat.Add(999, -999);
			}*/
		}
	}
}



