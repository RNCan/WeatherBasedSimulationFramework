//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
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
	static const double MAX_SDI = 6;
	//static const bool SDI_AGER = true; 

	static const double MIN_STRACH = 5;//Deslauriers data  
	static const double MAX_STRACH = 70;//Deslauriers data 
	static const double MIN_SUGAR = 7;//Deslauriers data 
	static const double MAX_SUGAR = 85;//Schaberg



	//static const double MAX_STRACH = 78;//Fabrizio
	//static const double MIN_STRACH = 0;//Fabrizio
	//static const double MAX_SUGAR = 324;//Fabrizio
	//static const double MIN_SUGAR = 0;//Fabrizio

	static const bool USE_SDI = true;
	static const bool USE_STARCH = true;
	static const bool USE_SUGAR = true;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSBWHostBudBurstModel::CreateObject);

	//const std::array < std::array<double, NB_SDI_PARAMS>, HBB::NB_SBW_SPECIES> CSBWHostBudBurstModel::SDI =
	//{ {
	//	{ 5.6553, 0.7802, 4.0, 50, -6, 26, 10.7, 0.7, 167, 9.47 },//BALSAM_FIR
	//	{ 5.6553, 0.7802, 4.0, 50, -6, 26, 10.7, 0.7, 167, 9.47 },//WHITE_SPRUCE
	//	{ 5.8284, 0.8331, 4.0, 50, -6, 26, 10.7, 0.7, 167, 9.47 },//BLACK_SPRUCE
	//	{ 5.6553, 0.7802, 4.0, 50, -6, 26, 10.7, 0.7, 167, 9.47 },//NORWAY_SPUCE
	//	{ 5.6553, 0.7802, 4.0, 50, -6, 26, 10.7, 0.7, 167, 9.47 },//RED_SPRUCE
	//} };

	CSBWHostBudBurstModel::CSBWHostBudBurstModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2021)";

		//m_SDI = SDI[0];
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
			m_P.S_sigma = parameters[c++].GetReal();
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
			m_P.S_min = parameters[c++].GetReal();
			m_P.S_max = parameters[c++].GetReal();
			m_P.S_mu = parameters[c++].GetReal();
			m_P.BB_thr = parameters[c++].GetReal();
			m_P.St_min = parameters[c++].GetReal();

			m_P.PAR_PS1 = parameters[c++].GetReal();
			m_P.PAR_PS2 = parameters[c++].GetReal();
			m_P.PAR_PS3 = parameters[c++].GetReal();
			m_P.PAR_SLA = parameters[c++].GetReal();
			m_P.m_version = HBB::TVersion(parameters[c++].GetInt());

			std::array<double, NB_SDI_PARAMS> SDI;
			for (size_t i = 0; i < NB_SDI_PARAMS; i++)
				SDI[i] = parameters[c++].GetReal();

			m_P.SDI_mu = SDI[μ];
			m_P.SDI_sigma = SDI[ѕ];
			m_P.G_v2 = SDI[ʎb];
			m_P.C_min = SDI[Τᴴ¹];
			m_P.C_max = SDI[Τᴴ²];
			m_P.PAR_minT = SDI[ʎ0];
			m_P.PAR_optT = SDI[ʎ1];
			m_P.PAR_maxT = SDI[ʎ2];
			m_P.m_nbSteps = SDI[ʎ3];
			
			m_bUseDefoliation = SDI[ʎa] != 0;

			//m_P.Sw_kk = m_SDI[2];
			//m_P.B_kk = m_SDI[3];
			//m_P.G_kk1 = m_SDI[4];
			//m_P.G_kk2 = m_SDI[5];
			//m_P.Mob_kk1 = m_SDI[6];
			//m_P.Acc_kk = m_SDI[7];
			//m_P.FH_kk1 = m_SDI[8];
			//m_P.FD_kk1 = m_SDI[9];


			ASSERT(m_species < HBB::PARAMETERS[0].size());


		}
		else
		{
			HBB::TVersion version = HBB::TVersion(parameters[c++].GetInt());
			m_P = HBB::PARAMETERS[version][m_species];
		}


		m_SDI_type = parameters[c++].GetInt();
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



		
		m_model.m_species = m_species;
		/*for (size_t y = 0; y < m_weather.size(); y++)
		{
			static const double DEFOL[5][11] =
			{
				{ 0,0,0,0,0,0,96.7,98.3,98.3,98.3 },
				{0},
				{0, 0, 0, 0, 0, 0, 8.3, 71.7, 71.7, 71.7},
				{0},
				{0},
			};

			model.m_defioliation[m_weather[y].GetTRef().GetYear()] = DEFOL[m_species][y]/100.0;

		}*/

		for (size_t y = 0; y < m_weather.size(); y++)
			m_model.m_defioliation[m_weather[y].GetTRef().GetYear()-1] = m_defoliation;



		bool bModelEx = m_info.m_modelName.find("Ex") != string::npos;


		m_model.m_P = m_P;
		//model.m_SDI = m_SDI;
		m_model.m_SDI_type = (TSDI)m_SDI_type;

		msg = m_model.Execute(m_weather, m_output, bModelEx);


		return msg;
	}











	enum { I_SPECIES1, I_SOURCE1, I_SITE1, I_DATE1, I_SDI1, I_N1, NB_INPUTS1 };
	enum { I_SPECIES2, I_SOURCE2, I_SITE2, I_DATE2, I_STARCH2, I_SUGAR2, I_SDI2, I_N2, I_DEFOL2, I_PROVINCE2, I_TYPE2, NB_INPUTS2 };


	void CSBWHostBudBurstModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns", "rs", "rbs" };

		if (data.size() == NB_INPUTS1)
		{
			if (data[I_SPECIES1] == SPECIES_NAME[m_species])
			{

				CSAResult obs;

				if (!m_bUseDefoliation || stod(data[I_DEFOL2]) > -999)
				{
					obs.m_ref.FromFormatedString(data[I_DATE1]);
					obs.m_obs[0] = stod(data[I_SDI1]);
					obs.m_obs.push_back(stod(data[I_N1]));
					obs.m_obs.push_back(stod(data[I_DEFOL2]));

					if (obs.m_obs[0] > -999)
					{
						m_years.insert(obs.m_ref.GetYear());
					}
					if (obs.m_obs[2] > -999)//defoliation
					{
						if (m_defioliation_by_year.find(obs.m_ref.GetYear()) != m_defioliation_by_year.end())
						{
							//defoliation for a site must be always the same
							ASSERT(m_defioliation_by_year[obs.m_ref.GetYear()] == obs.m_obs[2] / 100.0);
						}

						m_defioliation_by_year[obs.m_ref.GetYear()] = obs.m_obs[2] / 100.0;
					}
					


					m_SAResult.push_back(obs);
				}
			}
		}
		else if (data.size() == NB_INPUTS2)
		{
			if (data[I_SPECIES2] == SPECIES_NAME[m_species] && data[I_TYPE2] == "C")
			{
				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE2]);
				obs.m_obs[0] = stod(data[I_SDI2]);
				obs.m_obs.push_back(stod(data[I_STARCH2]));
				obs.m_obs.push_back(stod(data[I_SUGAR2]));
				obs.m_obs.push_back(stod(data[I_DEFOL2]));
				
				if ((USE_SDI && obs.m_obs[0] > -999) ||
					(USE_STARCH && obs.m_obs[1] > -999) ||
					(USE_SUGAR && obs.m_obs[2] > -999))
				{
					m_years.insert(obs.m_ref.GetYear());
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


				m_SAResult.push_back(obs);
			}
		}

	}

	//double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	//{
	//	double x = -999;

	//	if (obs > -999)
	//	{
	//		//if (obs > 0.01 && obs < 99.99)
	//		if (obs >= 100)
	//			obs = 99.99;//to avoid some problem of truncation

	//		long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
	//		if (index >= 1)
	//		{
	//			double obsX1 = output.GetFirstTRef().GetJDay() + index;
	//			double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

	//			double obsY1 = output[index][s];
	//			double obsY2 = output[index + 1][s];
	//			if (obsY2 != obsY1)
	//			{
	//				double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
	//				double obsX = obsX1 + (obs - obsY1) * slope;
	//				ASSERT(!_isnan(obsX) && _finite(obsX));

	//				x = obsX;
	//			}
	//		}
	//	}

	//	return x;
	//}




	static const int MAX_STAGE = 6;
	static const int ROUND_VAL = 4;
	void CSBWHostBudBurstModel::CalibrateSDI(CStatisticXY& stat)
	{

		//if (m_SDI[Τᴴ¹] >= m_SDI[Τᴴ²])
		//	return;


		//if (m_SAResult.empty())
		//	return;

		//if (!m_weather.IsHourly())
		//	m_weather.ComputeHourlyVariables();



		//for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		//{
		//	int year = m_weather[y].GetTRef().GetYear();
		//	if (m_years.find(year) == m_years.end())
		//		continue;



		//	double sumDD = 0;
		//	vector<double> CDD;
		//	CTPeriod p;


		//	p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		//	CDD.resize(p.size(), 0);
		//	CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_SDI[Τᴴ¹], m_SDI[Τᴴ²]);

		//	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		//	{
		//		const CWeatherDay& wday = m_weather.GetDay(TRef);
		//		size_t ii = TRef - p.Begin();
		//		sumDD += DDModel.GetDD(wday);
		//		CDD[ii] = sumDD;
		//	}




		//	for (size_t i = 0; i < m_SAResult.size(); i++)
		//	{
		//		size_t ii = m_SAResult[i].m_ref - p.Begin();
		//		if (m_SAResult[i].m_ref.GetYear() == year && ii < CDD.size())
		//		{
		//			double obs_y = m_SAResult[i].m_obs[0];

		//			if (obs_y > -999)
		//			{

		//				double sim_y = 0;


		//				//boost::math::logistic_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);
		//				boost::math::weibull_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);
		//				sim_y = Round(cdf(SDI_dist, CDD[ii]) * MAX_STAGE, ROUND_VAL);
		//				//double sim = m_SDI[ʎa] - m_SDI[ʎb] * cdf(begin_dist, sumDD), 0);



		//				if (sim_y < 0.05)
		//					sim_y = 0;
		//				if (sim_y > MAX_STAGE - 0.05)
		//					sim_y = MAX_STAGE;

		//				stat.Add(obs_y, sim_y);
		//			}
		//		}
		//	}
		//}//for all years


	}



	void CSBWHostBudBurstModel::GetFValueDaily(CStatisticXY& stat)
	{


		//return CalibrateSDI(stat);

		if (!m_P.is_valid())
			return;

		if (!m_SAResult.empty())
		{

			if (data_weather.GetNbYears() == 0)
			{
				CTPeriod pp(CTRef((*m_years.begin()) - 1, JANUARY, DAY_01), CTRef(*m_years.rbegin(), DECEMBER, DAY_31));
				pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));
				if (pp.IsInit())
				{
					((CLocation&)data_weather) = m_weather;
					data_weather.SetHourly(m_weather.IsHourly());
					//data_weather.CreateYears(pp);

					for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
					{
						data_weather[year] = m_weather[year];
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
				data_weather.ComputeHourlyVariables();

			CTPeriod pp(data_weather.GetEntireTPeriod(CTM::DAILY));


			//for (size_t yy = 1; yy < data_weather.GetNbYears(); yy++)
			//{
				//int year = data_weather[yy].GetTRef().GetYear();
				//if (m_years.find(year) == m_years.end())
					//continue;


			//CWeatherStation weather(m_weather.IsHourly());
			//((CLocation&)weather) = m_weather;
			//weather.CreateYears(year - 1, 2);
			//weather[year - 1] = m_weather[year - 1];
			//weather[year] = m_weather[year];

			//number of year of defoliation must be the same but with one year lag. Ex. weather = 2015-2020, def=2014-2019
			//ASSERT(m_defioliation_by_year.size()== data_weather.size());
			
			m_model.m_species = m_species;
			if(m_bUseDefoliation)
				m_model.m_defioliation = m_defioliation_by_year;
			//for (size_t y = 0; y < data_weather.size(); y++)
			//{
				//model.m_defioliation[data_weather[y].GetTRef().GetYear()] = m_defoliation;
			//}
				

			m_model.m_P = m_P;
			//model.m_SDI = m_SDI;
			m_model.m_SDI_type = (TSDI)m_SDI_type;

			CModelStatVector output;
			m_model.Execute(data_weather, output);

			//if (m_SAResult.front().m_obs.size() != 2)
			//{
			// 
			size_t nbInvalidS = 0;
			//	size_t nbInvalidSt = 0;
				//verify S_conc and St_conc and return if invalid
			for (size_t d = 0; d < output.size(); d++)
			{
				//&& S_invalid_factor >0&& St_invalid_factor>0
				if (_isnan(output[d][O_S_CONC]) || output[d][O_S_CONC] > 150 ||
					_isnan(output[d][O_ST_CONC]) || output[d][O_ST_CONC] > 150)//||)
					//_isnan(output[d][O_PS]) || output[d][O_PS] > 50 ||
					//_isnan(output[d][O_GROWTH_BDW_NDW]) || output[d][O_GROWTH_BDW_NDW] > 20 ||
				{
					nbInvalidS++;
				}
				/*	if (output[d][O_ST_CONC] > 2 * MAX_STRACH)
					{
						nbInvalidSt++;
					}*/
			}
			//}

			//if ( (100.0*nbInvalidS/ output.size()) > m_SAResult.size()/2)
				//return;
			size_t last_day = 213;
			//if (m_P.m_version == HBB::FABRIZIO_MODEL_NEW)
				//last_day = 243;


			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref))
				{
					if (m_SAResult.front().m_obs.size() == 2)
					{
						if (m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < last_day && output[m_SAResult[i].m_ref][O_SDI] > -999)
						{
							double obs_SDI = Round(m_SAResult[i].m_obs[0], 2);
							double sim_SDI = Round(output[m_SAResult[i].m_ref][O_SDI], 2);
							//sim_SDI = exp(log(5) * (sim_SDI - 3) / (6 - 3));

							bool bTest = i < min(nbInvalidS, m_SAResult.size() / 2);
							if (nbInvalidS > 0 && bTest)
								sim_SDI = Rand(-5.0, 0.0);


							//for(size_t n=0; n< m_SAResult[i].m_obs[1]; n++)
							stat.Add(obs_SDI, sim_SDI);
						}
					}
					else
					{

						if (USE_SDI && m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < last_day && output[m_SAResult[i].m_ref][O_SDI] > -999)
						{

							double maxSDI = m_SDI_type == SDI_DHONT ? 6 : 5;
							double obs_SDI = (m_SAResult[i].m_obs[0] - MIN_SDI) / (MAX_SDI - MIN_SDI);
							double sim_SDI = (output[m_SAResult[i].m_ref][O_SDI] - MIN_SDI) / (MAX_SDI - MIN_SDI);


							if (_isnan(sim_SDI) || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
								sim_SDI = Rand(-1.0, 0.0);


							stat.Add(obs_SDI, sim_SDI);
						}

						if (USE_STARCH && m_SAResult[i].m_obs[1] > -999 && output[m_SAResult[i].m_ref][O_ST_CONC] > -999)
						{
							double sim_St_conc = output[m_SAResult[i].m_ref][O_ST_CONC];// / output[m_SAResult[i].m_ref][O_BRANCH];
							double obs_starch = (m_SAResult[i].m_obs[1] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);
							double sim_starch = (sim_St_conc - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);

							if (_isnan(sim_starch) || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
								sim_starch = Rand(-1.0, 0.0);


							//double obs_starch = Round((m_SAResult[i].m_obs[1] - m_P.St_min) / (m_P.St_max - m_P.St_min), ROUND_VAL);
							//double sim_starch = Round((sim_St_conc - m_P.St_min) / (m_P.St_max - m_P.St_min), ROUND_VAL);
							//for(size_t i=0; i<10; i++)
							stat.Add(obs_starch, sim_starch);
						}


						if (USE_SUGAR && m_SAResult[i].m_obs[2] > -999 && output[m_SAResult[i].m_ref][O_S_CONC] > -999)
						{
							//double Smax = m_P.S_max + (m_P.S_min - P.S_max) / (1 + exp(-m_P.S_sigma * (I.Tmax14Days - P.S_mu)));

							double sim_S_conc = output[m_SAResult[i].m_ref][O_S_CONC];// / output[m_SAResult[i].m_ref][O_BRANCH];
							double obs_GFS = (m_SAResult[i].m_obs[2] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);
							double sim_GFS = (sim_S_conc - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);


							if (_isnan(sim_GFS) || (nbInvalidS > 0 && i < min(nbInvalidS, m_SAResult.size() / 2)))
								sim_GFS *= Rand(-1.0, 0.0);

							//double obs_GFS = max(0.0, min(1.0, Round((m_SAResult[i].m_obs[2] - m_P.S_min) / (m_P.S_max - m_P.S_min), ROUND_VAL)));
							//double sim_GFS = max(0.0, min(1.0, Round((sim_S_conc - m_P.S_min) / (m_P.S_max - m_P.S_min), ROUND_VAL)));
							//double obs_GFS = Round((m_SAResult[i].m_obs[2] - m_P.S_min) / (m_P.S_max - m_P.S_min), ROUND_VAL);
							//double sim_GFS = Round((sim_S_conc - m_P.S_min) / (m_P.S_max - m_P.S_min), ROUND_VAL);
							//for (size_t i = 0; i < 10; i++)
							stat.Add(obs_GFS, sim_GFS);
						}
					}
				}
			}//for all results
		//}
		}
	}




}