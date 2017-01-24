//*****************************************************************************
// Class: CSpruceBudwormDispersal
//
// Description: CSpruceBudwormDispersal is a BioSIM model for Spruce budworm dispersal
//*****************************************************************************
// 05/01/2016	1.0.0	Rémi Saint-Amant    Creation from old code
//*****************************************************************************
#include "Basic/ModelStat.h"
#include "Basic/UtilStd.h"
#include "ModelBase/EntryPoint.h"
#include "SpruceBudwormDispersal.h"
#include "SpruceBudworm.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{
	using namespace SBW;

	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBudwormDispersal::CreateObject);

	enum Toutput { O_EM, O_EF, O_LM, O_LF, O_AM, O_AF, O_MM, O_MF, O_GF, O_T, O_P, O_W, O_S, NB_OUTPUTS };
	extern char HOURLY_HEADER[] = "T,P,Ws,nbMales,nbFemales,Lm,Lf,Am,Af,Mm,Mf,Gf,sunset";

	class CBugStat
	{
	public:
		

		CBugStat(size_t sex, size_t L, double A, double M, double G, double T, double P, double W, double S)
		{
			m_sex=sex;
			m_L=L;
			m_A=A;
			m_M=M;
			m_G=sex==FEMALE?G:-999;
			m_T = T;
			m_P = P;
			m_W = W;
			m_S = S;
		}

		size_t m_sex;
		size_t m_L;
		double m_A;
		double m_M;
		double m_G;
		double m_T;
		double m_P;
		double m_W;
		double m_S;

	};
	
	typedef vector<CBugStat> CBugStatVector;
	typedef map<CTRef, CBugStatVector> CBugStatVectorMap;

	CSpruceBudwormDispersal::CSpruceBudwormDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2017)";
	}

	CSpruceBudwormDispersal::~CSpruceBudwormDispersal()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSpruceBudwormDispersal::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_nbMoths = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetFloat();

		return msg;
	}

	ERMsg CSpruceBudwormDispersal::OnExecuteHourly()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//CSun sun(m_weather.GetLocation().m_lat, m_weather.GetLocation().m_lon, m_weather.GetLocation().GetTimeZone());
		//

		////This is where the model is actually executed
		//m_output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output
		//for (size_t y = 0; y < m_weather.size(); y++)
		//{
		//	CTPeriod p = m_weather[y].GetEntireTPeriod();
		//	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		//		if (TRef.GetHour() == 0)
		//		{
		//			m_output[TRef][O_W] = (sun2.GetSunset(TRef) + 1.0);//+1 hours : assume to be in daylight zone  //[s]
		//			m_output[TRef][O_S] = (sun.GetSunset(TRef) + 1.0);//+1 hours : assume to be in daylight zone  //[s]
		//		}
		//			
		//	
		//}

		//return msg;



		CTPeriod overallPeriod;

		CBugStatVectorMap flyers;

		//for all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			//CTStatMatrix annualStat(p, NB_OUTPUTS);

			//Create stand
			CSBWStand stand(this);
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			stand.m_defoliation = m_defoliation;

			CSBWTreePtr pTree(new CSBWTree(&stand));
			stand.m_host.push_front(pTree);


			//Create tree
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, NOT_INIT, false, 0));




			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				for (CSBWTree::iterator it = pTree->begin(); it != pTree->end(); it++)
				{
					CSpruceBudworm& budworm = static_cast<CSpruceBudworm&>(*(*it).get());
					if (budworm.IsAlive())
					{
						const CHourlyData& w = m_weather.GetHour(TRef);

						ASSERT(w.GetParent());
						const CWeatherDay& day° = (const CWeatherDay&)*w.GetParent();

						if (day°[H_TMIN2][MEAN] >= -10)
						{
							budworm.Live(w, 1);

							//compute brood and flight activity only once
							if (budworm.GetStage() == ADULT && w.GetTRef().GetHour() == 18)
							{

								if ((*it)->GetSex() == FEMALE)
									(*it)->Brood(day°);

								__int64 t° = 0;
								__int64 tᴹ = 0;

								if (budworm.get_t(day°, t°, tᴹ))
								{
									//calculate tᶜ
									__int64 tᶜ = (t° + tᴹ) / 2;

									//now compute tau, p and flight
									bool bExodus = false;
									static const __int64 Δt = 60;
									for (__int64 t = t°; t <= tᴹ && !bExodus; t += Δt)
									{
										double tau = double(t - tᶜ) / (tᴹ - tᶜ);

										double h = t / 3600.0;
										size_t L = size_t((h - size_t(h)) * 3600);
										//size_t h° = size_t(h);

										const CWeatherDay& day¹ = day°.GetNext();
										const CWeatherDay& w = h < 24 ? day° : day¹;

										double T = budworm.get_Tair(w, h < 24 ? h : h - 24.0);
										double P = budworm.get_Prcp(w, h < 24 ? h : h - 24.0);
										double WS = budworm.get_WndS(w, h < 24 ? h : h - 24.0); 

										bExodus = budworm.ComputeExodus(T, P, WS, tau);
										if (bExodus)
										{
											CSun sun(day°.GetLocation().m_lat, day°.GetLocation().m_lon, day°.GetLocation().GetTimeZone());
											double sunset = (sun.GetSunset(day°.GetTRef()) + 1.0 );//+1 hour : assume to be in daylight zone  //[s]


											size_t sex = budworm.GetSex();
											CTRef TRefTmp = TRef + (size_t(h) - TRef.GetHour());
											overallPeriod += TRefTmp;
											flyers[TRefTmp].push_back(CBugStat(sex, L, budworm.GetA(), budworm.GetM(), budworm.GetG(), T, P, WS, sunset));

											budworm.SetStatus(CIndividual::DEAD);
											budworm.SetDeath(CIndividual::EXODUS);
										}//if exodus occurd
									}//for t in exodus period
								}//if exodus occur
							}//if adult

							budworm.Die(day°);
						}//temperature is over -10 °C
					}//if is alive
				}//for all insect 

				HxGridTestConnection();
			}//for all hours of a year
		}//for all years

		//overallPeriod.Transform(CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR));
		m_output.Init(overallPeriod, NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output

		////copy stat to output
		//for (int sex = 0; sex < 2; sex++)
		//{
		//	for (CBugStatVectorMap::iterator it = flyers[sex].begin(); it != flyers[sex].end(); it++)
		//	{
		//		static const double SEX_RATIO[2] = { 1.0, 1.0 };
		//		const CBugStatVector& bugs = it->second;
		//		//randomly select an insect
		//		//size_t sel = m_randomGenerator.Rand(0, int(bugs.size()) - 1);

		//		//CTRef TRef = it->first;
		//		//m_output[TRef][O_MALES_EXODUS + sex] = bugs.size()*SEX_RATIO[sex];
		//		//m_output[TRef][O_LM + sex] = bugs[sel].m_L;
		//		//m_output[TRef][O_AM + sex] = bugs[sel].m_A;
		//		//m_output[TRef][O_MM + sex] = bugs[sel].m_M;
		//		//m_output[TRef][O_GF] = (sex == FEMALE) ? bugs[sel].m_G : -999;

		//		array<CStatistic, NB_OUTPUTS> stats;
		//		//size_t sel = m_randomGenerator.Rand(0, int(bugs.size()) - 1);
		//		for (size_t i = 0; i < bugs.size(); i++)
		//		{
		//			stats[O_MALES_EXODUS + sex] += bugs.size()*SEX_RATIO[sex];
		//			stats[O_LM + sex] += bugs[i].m_L;
		//			stats[O_AM + sex] += bugs[i].m_A;
		//			stats[O_MM + sex] += bugs[i].m_M;
		//			if (sex == FEMALE)
		//				stats[O_GF] += bugs[i].m_G;
		//			
		//			stats[O_T] += bugs[i].m_T;
		//			stats[O_P] += bugs[i].m_P;
		//			stats[O_W] += bugs[i].m_W;
		//			stats[O_SUNSET] += bugs[i].m_S;
		//		}

		//		CTRef TRef = it->first;
		//		m_output[TRef][O_MALES_EXODUS + sex] = stats[O_MALES_EXODUS + sex];
		//		m_output[TRef][O_LM + sex] = stats[;
		//		m_output[TRef][O_AM + sex] = stats[;
		//		m_output[TRef][O_MM + sex] = stats[;
		//		m_output[TRef][O_GF] = (sex == FEMALE) ? bugs[sel].m_G : -999;

		//		

		//		//m_output[TRef][O_MALES_EXODUS + sex] = bugs.size()*SEX_RATIO[sex];
		//		//m_output[TRef][O_LM + sex] = bugs[sel].m_L;
		//		//m_output[TRef][O_AM + sex] = bugs[sel].m_A;
		//		//m_output[TRef][O_MM + sex] = bugs[sel].m_M;
		//		//m_output[TRef][O_GF] = (sex == FEMALE) ? bugs[sel].m_G : -999;


		//		//set the other sex to zer0 when not init
		//		if (m_output[TRef][(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] == -999)
		//			m_output[TRef][(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] = 0;
		//		
		//	}
		//}

		//copy stat to output
		for (CBugStatVectorMap::iterator it = flyers.begin(); it != flyers.end(); it++)
		{
			//for (int sex = 0; sex < 2; sex++)
			{

				static const double SEX_RATIO[2] = { 1.0, 1.0 };
				const CBugStatVector& bugs = it->second;
				//randomly select an insect
				//size_t sel = m_randomGenerator.Rand(0, int(bugs.size()) - 1);

				//CTRef TRef = it->first;
				//m_output[TRef][O_MALES_EXODUS + sex] = bugs.size()*SEX_RATIO[sex];
				//m_output[TRef][O_LM + sex] = bugs[sel].m_L;
				//m_output[TRef][O_AM + sex] = bugs[sel].m_A;
				//m_output[TRef][O_MM + sex] = bugs[sel].m_M;
				//m_output[TRef][O_GF] = (sex == FEMALE) ? bugs[sel].m_G : -999;

				array<CStatistic, NB_OUTPUTS> stats;
				//size_t sel = m_randomGenerator.Rand(0, int(bugs.size()) - 1);
				for (size_t i = 0; i < bugs.size(); i++)
				{
					size_t sex = bugs[i].m_sex;
					stats[O_EM + sex] += 1;
					stats[O_EM + (sex ? 1 : 0)] += 0;
					stats[O_LM + sex] += bugs[i].m_L;
					stats[O_AM + sex] += bugs[i].m_A;
					stats[O_MM + sex] += bugs[i].m_M;
					if (sex == FEMALE)
						stats[O_GF] += bugs[i].m_G;

					stats[O_T] += bugs[i].m_T;
					stats[O_P] += bugs[i].m_P;
					stats[O_W] += bugs[i].m_W;
					stats[O_S] += bugs[i].m_S;
				}

				CTRef TRef = it->first;
				for (size_t i = 0; i < NB_OUTPUTS; i++)
					m_output[TRef][i] = stats[i][i < 2 ? SUM : MEAN];

				//set the other sex to zer0 when not init
				//if (m_output[TRef][(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] == -999)
				//m_output[TRef][(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] = 0;

			}
		}


		return msg;
	}

}