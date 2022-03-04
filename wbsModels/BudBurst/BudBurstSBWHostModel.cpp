//*********************************************************************
//2022-02-15	1.0.3	Rémi Saint-Amant	Add branch weight in calibration, new min/max
//2022-02-01	1.0.2	Rémi Saint-Amant	Compile with new model
//2021-01-01	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstSBWHostModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	static const double MIN_SDI = 0;
	static const double MAX_SDI = 5;
	static const double MIN_SDI_DOY = 0.25;
	static const double MAX_SDI_DOY = 4.75;
	//static const double MIN_STRACH = 4.9;//from data   [1,99]
	//static const double MAX_STRACH = 116.0;//from data   [1,99]
	//static const double MIN_SUGAR = 8.2;//from data   [1,99] 
	//static const double MAX_SUGAR = 87.8;//from data   [1,99]
	//static const double MIN_MASS = 0.01506;//from data   [1,99]
	//static const double MAX_MASS = 0.1592;//from data   [1,99]
	//static const double MAX_DOY_DATA = 213-1;


	static const bool USE_SDI = true;
	static const bool USE_STARCH = true;
	static const bool USE_SUGAR = true;
	static const bool USE_MASS = true;
	static const bool USE_LENGTH = true;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSBWHostBudBurstModel::CreateObject);


	CSBWHostBudBurstModel::CSBWHostBudBurstModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.3 (2022)";
		m_SDI_type = SDI_AUGER;
		m_nbSteps = 1;
	}

	CSBWHostBudBurstModel::~CSBWHostBudBurstModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CSBWHostBudBurstModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg; 

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetReal() / 100.0;

		ASSERT(m_species < HBB::PARAMETERS[0].size());

		m_P = HBB::PARAMETERS[0][m_species];

		double test = 0.000000012;
		string str1 = ToString(test, 10);
		string str2 = ToString(test);



		//m_SDI = SDI[m_species];
		if (parameters.size() == 2 + 46 + NB_SDI_PARAMS + 1)
		{
			m_P.Sw_v = parameters[c++].GetReal();
			m_P.Sw_k = parameters[c++].GetReal();
			m_P.Sw_eff = parameters[c++].GetReal();
			m_P.Mob_v1 = parameters[c++].GetReal();
			m_P.Mob_k1 = parameters[c++].GetReal();
			m_P.FH_v1 = parameters[c++].GetReal();
			m_P.FH_k1 = parameters[c++].GetReal();
			m_P.Acc_v = parameters[c++].GetReal();
			m_P.Acc_k = parameters[c++].GetReal();
			m_P.Mob_v2 = parameters[c++].GetReal();
			m_P.Mob_k2 = parameters[c++].GetReal();
			m_P.FH_v2 = parameters[c++].GetReal();
			m_P.FH_k2 = parameters[c++].GetReal();
			m_P.G_v1 = parameters[c++].GetReal();
			m_P.G_k1 = parameters[c++].GetReal();
			m_P.G_k2 = parameters[c++].GetReal();
			m_P.FH_sigma = parameters[c++].GetReal();
			m_P.St_max = parameters[c++].GetReal();
			m_P.G_minT = parameters[c++].GetReal();
			m_P.G_optT = parameters[c++].GetReal();
			m_P.G_maxT = parameters[c++].GetReal();
			m_P.I_c = parameters[c++].GetReal();
			m_P.FD_v1 = parameters[c++].GetReal();
			m_P.FD_k1 = parameters[c++].GetReal();
			m_P.FD_v2 = parameters[c++].GetReal();
			m_P.FD_k2 = parameters[c++].GetReal();
			m_P.FD_muS = parameters[c++].GetReal();
			m_P.M_minT = parameters[c++].GetReal();
			m_P.M_optT = parameters[c++].GetReal();
			m_P.M_maxT = parameters[c++].GetReal();
			m_P.F_minT = parameters[c++].GetReal();
			m_P.F_optT = parameters[c++].GetReal();
			m_P.F_maxT = parameters[c++].GetReal();
			m_P.B_v = parameters[c++].GetReal();
			m_P.B_k = parameters[c++].GetReal();
			m_P.B_eff = parameters[c++].GetReal();
			m_P.cS_min = parameters[c++].GetReal();
			m_P.cS_max = parameters[c++].GetReal();
			//	m_P.S_mu = 
			parameters[c++].GetReal();
			m_P.BB_thr = parameters[c++].GetReal();
			//m_P.St_min = 
			parameters[c++].GetReal();

			m_P.PAR_PS1 = parameters[c++].GetReal();
			m_P.PAR_PS2 = parameters[c++].GetReal();
			m_P.PAR_PS3 = parameters[c++].GetReal();
			m_P.PAR_SLA = parameters[c++].GetReal();
			m_version = HBB::TVersion(parameters[c++].GetInt());


			std::array<double, NB_SDI_PARAMS> SDI;
			for (size_t i = 0; i < NB_SDI_PARAMS; i++)
				SDI[i] = parameters[c++].GetReal();

			m_P.SDI_mu = SDI[μ];
			m_P.SDI_sigma = SDI[ѕ];
			m_P.Bdw_0 = SDI[ʎb];
			//m_P.C_min = SDI[Τᴴ¹];
			//m_P.C_max = SDI[Τᴴ²];
			m_P.PAR_minT = SDI[ʎ0];
			m_P.PAR_optT = SDI[ʎ1];
			m_P.PAR_maxT = SDI[ʎ2];
			//m_P.
			m_nbSteps = SDI[ʎ3];

			m_bUseDefoliation = SDI[ʎa] != 0;

			//m_P.Sw_kk = m_SDI[2];
			//m_P.B_kk = m_SDI[3];
			//m_P.G_kk1 = m_SDI[4];
			//m_P.G_kk2 = m_SDI[5];
			//m_P.Mob_kk1 = m_SDI[6];
			//m_P.Acc_kk = m_SDI[7];
			//m_P.FH_kk1 = m_SDI[8];
			//m_P.FD_kk1 = m_SDI[9];


			//if (version == HBB::V_ORIGINAL)
			//{
			//	m_P = HBB::PARAMETERS[HBB::V_ORIGINAL][m_species];
			//	m_P.SDI_mu = SDI[μ];
			//	m_P.SDI_sigma = SDI[ѕ];
			//}

			ASSERT(m_species < HBB::PARAMETERS[0].size());


		}
		else
		{
			m_version = HBB::TVersion(parameters[c++].GetInt());
			m_P = HBB::PARAMETERS[m_version][m_species];

			if (m_version == HBB::V_ORIGINAL)
				m_nbSteps = 10;
		}




		//m_SDI_type = parameters[c++].GetInt();// no longer support Dhont.
		m_SDI_type = SDI_AUGER;
		ASSERT(m_SDI_type < NB_SDI_TYPE);
		//ASSERT(m_P == HBB::PARAMETERS[m_P.m_version][m_species]);


		return msg;
	}


	//This method is call to compute solution
	ERMsg CSBWHostBudBurstModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();




		//m_model.m_species = m_species;
		//for (size_t y = 0; y < m_weather.size(); y++)
		//{
		//	static const double DEFOL[4][10] =
		//	{
		//		{ 0,0,0,0,0,0,96.7,98.3,98.3,98.3 },
		//		{0},
		//		{0, 0, 0, 0, 0, 0, 8.3, 71.7, 71.7, 71.7},
		//		{0},
		//	};
		//
		//	m_model.m_defioliation[m_weather[y].GetTRef().GetYear()] = DEFOL[m_species][y]/100.0;
		//
		//}

		for (size_t y = 0; y < m_weather.size(); y++)
			m_model.m_defioliation[m_weather[y].GetTRef().GetYear()] = m_defoliation;



		bool bModelEx = m_info.m_modelName.find("Ex") != string::npos;

		m_model.m_species = m_species;
		m_model.m_SDI_type = (TSDI)m_SDI_type;
		m_model.m_nbSteps = m_nbSteps;
		m_model.m_P = m_P;
		m_model.m_version = m_version;
		//model.m_SDI = m_SDI;
		

		msg = m_model.Execute(m_weather, m_output, bModelEx);


		return msg;
	}











	
	enum { I_SPECIES, I_SOURCE, I_SITE, I_LATITUDE, I_LONGITUDE, I_ELEVATION, I_DATE, I_STARCH, I_SUGAR, I_B_LENGTH, I_B_MASS, I_N_MASS, I_SDI, I_N, I_DEF, I_DEF_END_N1, I_DEF_END_N, I_PROVINCE, I_TYPE, NB_INPUTS };

	void CSBWHostBudBurstModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns", "rs", "rbs" };

		
		if (data.size() == NB_INPUTS)
		{
			if (data[I_SPECIES] == SPECIES_NAME[m_species] && data[I_TYPE] == "C")
			{
				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE]);
				obs.m_obs[0] = stod(data[I_SDI]);
				obs.m_obs.push_back(stod(data[I_STARCH]));
				obs.m_obs.push_back(stod(data[I_SUGAR]));
				obs.m_obs.push_back(stod(data[I_B_MASS]));
				obs.m_obs.push_back(stod(data[I_B_LENGTH]));
				

				if ((USE_SDI && obs.m_obs[0] > -999) ||
					(USE_STARCH && obs.m_obs[1] > -999) ||
					(USE_SUGAR && obs.m_obs[2] > -999) ||
					(USE_MASS && obs.m_obs[3] > -999) || 
					(USE_LENGTH && obs.m_obs[4] > -999) )
				{
					if(obs.m_ref.GetJDay()<213)
						m_years.insert(obs.m_ref.GetYear());
					else
						m_years.insert(obs.m_ref.GetYear()+1);

					m_SAResult.push_back(obs);
				}

				//if (obs.m_obs[3] > -999)//defoliation
				//{
				//	if (m_defioliation_by_year.find(obs.m_ref.GetYear()) != m_defioliation_by_year.end())
				//	{
				//		//defoliation for a site must be always the same
				//		ASSERT(m_defioliation_by_year[obs.m_ref.GetYear()] == obs.m_obs[3] / 100.0);
				//	}
				//
				//	m_defioliation_by_year[obs.m_ref.GetYear()] = obs.m_obs[3] / 100.0;
				//}


				
			}
		}

	}



	double GetSimDOY(const CModelStatVector& output, const CSAResult& result)
	{
		CTPeriod p(result.m_ref.GetYear(), JANUARY, DAY_01, result.m_ref.GetYear(), DECEMBER, DAY_31);
		int pos = output.GetFirstIndex(O_SDI, ">", result.m_obs[0], 0, p);
		return pos >= 0 ? (output.GetFirstTRef() + pos).GetJDay() : -999;
	}


	void CSBWHostBudBurstModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (!m_P.is_valid())
			return;

		if (!m_SAResult.empty())
		{
			if (!m_SDI_DOY_stat.IsInit())
			{
				const CSimulatedAnnealingVector& all_results = GetSimulatedAnnealingVector();

				for (auto it = all_results.begin(); it != all_results.end(); it++)
				{
					const CSAResultVector& results = (*it)->GetSAResult();
					for (auto iit = results.begin(); iit != results.end(); iit++)
					{
						if (iit->m_obs[0] >= MIN_SDI_DOY && iit->m_obs[0] <= MAX_SDI_DOY)
							m_SDI_DOY_stat += iit->m_ref.GetJDay();

						for (size_t i = 0; i < m_stat.size(); i++)
						{
							if(iit->m_obs[i]>-999)
								m_stat[i] += iit->m_obs[i];
						}
					}
				}
			}



			if (m_data_weather.GetNbYears() == 0)
			{
				CTPeriod pp(CTRef((*m_years.begin()) - 1, JANUARY, DAY_01), CTRef(*m_years.rbegin(), DECEMBER, DAY_31));
				pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));
				if (pp.IsInit())
				{
					((CLocation&)m_data_weather) = m_weather;
					m_data_weather.SetHourly(m_weather.IsHourly());
					//m_data_weather.CreateYears(pp);

					for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
					{
						m_data_weather[year] = m_weather[year];
					}
				}
				else
				{
					//remove these obs, no input weather
					m_SAResult.clear();
					return;
				}
			}


			if (!m_weather.IsHourly())
				m_data_weather.ComputeHourlyVariables();

			CTPeriod pp(m_data_weather.GetEntireTPeriod(CTM::DAILY));

			m_model.m_species = m_species;
			m_model.m_P = m_P;
			//m_model.m_SDI_type = (TSDI)m_SDI_type;
			m_model.m_version = m_version;

			CModelStatVector output;
			m_model.Execute(m_data_weather, output, false);


			size_t nbInvalidS = 0;

			for (size_t d = 0; d < output.size(); d++)
			{

				if (_isnan(output[d][O_S_CONC]) || output[d][O_S_CONC] > 250 ||
					_isnan(output[d][O_ST_CONC]) || output[d][O_ST_CONC] > 250 ||
					_isnan(output[d][O_BRANCH_MASS]) || output[d][O_BRANCH_MASS] > 0.5)
				{
					//return;
					nbInvalidS++;
				}
			}

			size_t max_doy_data = 213-1;
			if (m_version == HBB::V_MODIFIED)
				max_doy_data = 244-1;
			//size_t last_day = 213;

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref) && m_SAResult[i].m_ref.GetJDay() < max_doy_data)
				{
					if (USE_SDI && m_SAResult[i].m_obs[0] > -999 )
					{
						ASSERT(output[m_SAResult[i].m_ref][O_SDI] > -999);
						ASSERT(m_SAResult[i].m_ref.GetJDay() < 213);

						//double obs_SDI = (m_SAResult[i].m_obs[0] - MIN_SDI) / (MAX_SDI - MIN_SDI);
						//double sim_SDI = (output[m_SAResult[i].m_ref][O_SDI] - MIN_SDI) / (MAX_SDI - MIN_SDI);

						double obs_SDI = (m_SAResult[i].m_obs[0] - m_stat[0][LOWEST]) / m_stat[0][RANGE];
						double sim_SDI = (output[m_SAResult[i].m_ref][O_SDI] - m_stat[0][LOWEST]) / m_stat[0][RANGE];
						

						if (_isnan(sim_SDI) || output[m_SAResult[i].m_ref][O_SDI] == -999 || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
							sim_SDI = Rand(-1.0, 0.0);


						stat.Add(obs_SDI, sim_SDI);

						if (m_SAResult[i].m_obs[0] >= MIN_SDI_DOY && m_SAResult[i].m_obs[0] <= MAX_SDI_DOY)
						{
							double DOY = GetSimDOY(output, m_SAResult[i]);
							if (DOY > -999)
							{
								double obs_DOY = (m_SAResult[i].m_ref.GetJDay() - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
								double sim_DOY = (DOY - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];

								stat.Add(obs_DOY, sim_DOY);
							}
						}


					}

					if (USE_STARCH && m_SAResult[i].m_obs[1] > -999 )
					{
						ASSERT(output[m_SAResult[i].m_ref][O_ST_CONC] > -999);

						double obs_cSt = (m_SAResult[i].m_obs[1] - m_stat[1][LOWEST]) / m_stat[1][RANGE];
						double sim_cSt = (output[m_SAResult[i].m_ref][O_ST_CONC] - m_stat[1][LOWEST]) / m_stat[1][RANGE];

						//double obs_starch = (m_SAResult[i].m_obs[1] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);
						//double sim_starch = (output[m_SAResult[i].m_ref][O_ST_CONC] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);

						if (_isnan(sim_cSt) || output[m_SAResult[i].m_ref][O_ST_CONC] == -999 || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
							sim_cSt = Rand(-1.0, 0.0);

						//for (size_t j = 0; j < 5; j++)
							stat.Add(obs_cSt, sim_cSt);
					}


					if (USE_SUGAR && m_SAResult[i].m_obs[2] > -999 ) 
					{
						ASSERT(output[m_SAResult[i].m_ref][O_S_CONC] > -999);

						
						//double obs_GFS = (m_SAResult[i].m_obs[2] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);
						//double sim_GFS = (output[m_SAResult[i].m_ref][O_S_CONC] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);
						
						double obs_cS = (m_SAResult[i].m_obs[2] - m_stat[2][LOWEST]) / m_stat[2][RANGE];
						double sim_cS = (output[m_SAResult[i].m_ref][O_S_CONC] - m_stat[2][LOWEST]) / m_stat[2][RANGE];

						if (_isnan(sim_cS) || output[m_SAResult[i].m_ref][O_S_CONC] == -999 || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
							sim_cS *= Rand(-1.0, 0.0);

						//for (size_t j = 0; j < 5; j++)
							stat.Add(obs_cS, sim_cS);
					}


					if (USE_MASS && m_SAResult[i].m_obs[3] > -999 )
					{
						ASSERT(output[m_SAResult[i].m_ref][O_BRANCH_MASS] > -999);

						//Compare the mass gain
						//remove the first value of the branch (buds mass Mdw_0) from the branch mass
						//double Mdw_0 = output[CTRef(m_SAResult[i].m_ref.GetYear(), JANUARY, DAY_01)][O_BUDS_MASS]; 
						//double M = output[m_SAResult[i].m_ref][O_BUDS_MASS];
						//double BM = output[m_SAResult[i].m_ref][O_BRANCH_MASS] + M - Mdw_0;
						//
						//
						//double Bdw_0 = m_stat[3][LOWEST];
						//double obs_BM = (m_SAResult[i].m_obs[3] - Bdw_0) / (m_stat[3][RANGE] - Bdw_0);//Mass gain
						//double sim_BM = (BM - (m_stat[3][LOWEST]- Bdw_0) ) / (m_stat[3][RANGE]- Bdw_0);//mass gain


						//double Mdw_0 = output[CTRef(m_SAResult[i].m_ref.GetYear(), JANUARY, DAY_01)][O_BUDS_MASS];
						double M = output[m_SAResult[i].m_ref][O_BUDS_MASS];
						double B = output[m_SAResult[i].m_ref][O_BRANCH_MASS];
						double BM =  B + M;


						//double Bdw_0 = m_stat[3][LOWEST];
						double obs_BM = m_SAResult[i].m_obs[3] /m_stat[3][RANGE];//Mass gain
						double sim_BM = BM/m_stat[3][RANGE];//mass gain



						if (_isnan(sim_BM) || output[m_SAResult[i].m_ref][O_BUDS_MASS] == -999 || output[m_SAResult[i].m_ref][O_BRANCH_MASS] == -999 || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
							sim_BM *= Rand(-1.0, 0.0);

						//for (size_t j = 0; j < 5; j++)
							stat.Add(obs_BM, sim_BM);
					}

					if (USE_LENGTH && m_SAResult[i].m_obs[4] > -999)
					{
						ASSERT(output[m_SAResult[i].m_ref][O_BRANCH_LENGTH] > -999);
						
						double obs_BM = (m_SAResult[i].m_obs[4]- m_stat[4][LOWEST]) / m_stat[4][RANGE];//Mass gain
						double sim_BM = (output[m_SAResult[i].m_ref][O_BRANCH_LENGTH]- m_stat[4][LOWEST]) / m_stat[4][RANGE];//mass gain

						//for (size_t j = 0; j < 5; j++)
						stat.Add(obs_BM, sim_BM);
					}
				}

			}//for all results
		//}
		}
	}




}