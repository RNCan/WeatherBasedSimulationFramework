//*********************************************************************
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
//complet
		//{  -8.6, 2.0, 4.6, 6.3},
		//{  -2.3, 5.6, 8.8, 11.1 },
		//{   3.8, 9.3, 13.0, 15.9 },
		//{ 128.3, 691.7, 1124.3, 1755.4 },
		//{ 11.1, 14.6, 18.0, 20.8 },
		//{ -23.6, -4.2, 0.7, 3.9 },
		//{ -3.1, 10.6, 16.5, 20.7 },
		//{ -23.6, -2.2, 5.1, 17.2 }, 
		//{ 42.3, 130.4, 241.3, 563.1 },
		//{ 12.2, 53.9, 142.6, 467.4 },
		//{ 37.8, 158.3, 301.4, 511.3 },
		//{ 11.9, 49.7, 114.0, 169.4 },
		//{ 0.0, 29.5, 108.2, 239.8 },
		//{ -451.4, -94.7, 41.2, 152.3 },
		//{ -606.9, -196.2, -83.9, 0.2 },
		//{ -606.9, -272.1, -46.6, 120.3 },
		//{ -266.6, -94.7, -22.1, 166.0 },
		//{ 12.2, 15.9, 19.4, 21.7 },
		//{ -24.7, -6.4, -1.6, 2.3 },
		//{ 22.0, 116.9, 222.0, 360.3 },
		//{ 1.2, 7.9, 28.0, 56.0 },
		//{ 605.2, 1115.5, 1569.6, 1885.7 },

		//only water deficit
		/*{-8.6,2.0,4.6,7.3},
		{ -2.3, 5.6, 8.8, 11.8 },
		{ 3.8, 9.3, 13.0, 16.2 },
		{ 128.3, 691.7, 1124.3, 1816.4 },
		{ 11.1, 14.6, 18.0, 21.0 },
		{ -23.6, -4.2, 0.7, 4.1 },
		{ -3.1, 10.6, 16.5, 21.2 },
		{ -23.6, -2.2, 5.1, 18.2 },
		{ 42.3, 130.4, 241.3, 597.4 },
		{ 12.2, 53.9, 142.6, 490.1 },
		{ 37.8, 158.3, 301.4, 569.2 },
		{ 11.9, 49.7, 114.0, 176.4 },
		{ -1339.0, -637.5, -223.3, 339.5 },
		{ -451.4, -94.7, 41.2, 187.9 },
		{ -606.9, -196.2, -83.9, 8.9 },
		{ -606.9, -272.1, -46.6, 161.4 },
		{ -266.6, -94.7, -22.1, 177.7 },
		{ 12.2, 15.9, 19.4, 22.4 },
		{ -24.7, -6.4, -1.6, 2.6 },
		{ 22.0, 116.9, 222.0, 431.1 },
		{ 1.2, 7.9, 28.0, 63.5 },
		{ 605.2, 1115.5, 1569.6, 1984.7 },*/

		//wisper
		/*{ -1.0, 1.9, 4.6, 7.1 },
		{ 2.6, 5.7, 8.8, 11.7 },
		{ 6.3, 9.6, 13.0, 16.2 },
		{ 219.4, 690.1, 1124.3, 1522.0 },
		{ 11.6, 14.8, 18.1, 20.9 },
		{ -8.3, -4.4, 0.7, 4.1 },
		{ 6.7, 11.3, 16.5, 20.9 },
		{ -8.9, -1.9, 5.5, 13.4 },
		{ 42.3, 130.1, 238.3, 348.6 },
		{ 7.5, 53.9, 142.6, 225.9 },
		{ 37.8, 154.4, 302.8, 417.7 },
		{ 7.5, 49.4, 114.0, 177.2 },
		{ -1007.3, -633.9, -205.4, 278.1 },
		{ -224.3, -95.9, 41.8, 187.6 },
		{ -289.0, -192.5, -83.4, 10.4 },
		{ -518.2, -273.1, -35.0, 161.2 },
		{ -164.3, -93.8, -20.9, 54.7 },
		{ 12.1, 16.0, 19.5, 22.4 },
		{ -10.5, -6.5, -1.6, 2.6 },
		{ 22.0, 116.7, 222.0, 322.2 },
		{ 1.2, 7.6, 27.1, 45.7 },
		{ 644.4, 1128.2, 1569.6, 1968.9 },*/

		//10-90
		{  -8.8, 0.0, 5.7, 7.3 },
		{  -2.3, 3.8, 9.9, 11.7 },
		{   3.6, 8.2, 14.4, 16.2 },
		{ 128.3, 514.7, 1383.4, 1816.4 },
		{ 11.1, 13.9, 19.1, 20.9 },
		{ -23.1, -7.2, 2.2, 4.1 },
		{ -3.9, 3.3, 18.1, 20.9 },
		{ -23.1, -6.1, 11.9, 18.2 },
		{ 42.3, 105.0, 344.0, 597.4 },
		{ 7.5, 34.7, 214.8, 490.1 },
		{ 37.8, 123.8, 365.8, 569.2 },
		{ 7.5, 33.4, 139.5, 182.3 },
		{ -1311.1, -905.5, -40.4, 337.7 },
		{ -449.9, -167.6, 97.7, 187.6 },
		{ -606.4, -339.3, -48.8, 10.4 },
		{ -606.4, -356.2, 39.8, 161.2 },
		{ -266.6, -135.3, 43.0, 178.4 },
		{ 12.0, 14.7, 20.4, 22.4 },
		{ -24.3, -10.1, 0.3, 2.6 },
		{ 22.0, 87.3, 287.9, 431.1 },
		{ 1.2, 3.7, 45.5, 63.5 },
		{ 593.0, 960.3, 1769.0, 1983.5 },

		//10-90 AR
	/*	{ -8.8, 0.0, 5.7, 7.3 },
		{ -2.3, 3.8, 9.9, 11.7 },
		{ 3.6, 8.2, 14.4, 16.2 },
		{ 128.3, 514.7, 1383.4, 1816.4 },
		{ 11.1, 13.9, 19.1, 20.9 },
		{ -23.1, -7.2, 2.2, 4.1 },
		{ -3.9, 3.3, 18.1, 20.9 },
		{ -23.1, -6.1, 11.9, 18.2 },
		{ 42.3, 105.0, 344.0, 597.4 },
		{ 7.5, 34.7, 214.8, 490.1 },
		{ 37.8, 123.8, 365.8, 569.2 },
		{ 7.5, 33.4, 139.5, 182.3 },
		{ 0.0, 19.3, 160.2, 357.8 },
		{ 0.0, 0.0, 114.4, 187.6 },
		{ 0.0, 0.0, 2.7, 18.2 },
		{ 0.0, 0.0, 60.1, 161.2 },
		{ 0.0, 0.0, 59.4, 178.4 },
		{ 12.0, 14.7, 20.4, 22.4 },
		{ -24.3, -10.1, 0.3, 2.6 },
		{ 22.0, 87.3, 287.9, 431.1 },
		{ 1.2, 3.7, 45.5, 63.5 },
		{ 593.0, 960.3, 1769.0, 1983.5 },
*/

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
		return CLimits(F / nbVars, pow(max(1.0, F), 1.0 / nbVars));
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
		VERSION = "1.1.1 (2016)";

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



