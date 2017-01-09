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

	enum Toutput { O_MALES_EXODUS, O_FEMALES_EXODUS, O_LM, O_LF, O_AM, O_AF, O_MM, O_MF, O_GF, NB_OUTPUTS };
	extern char HOURLY_HEADER[] = "nbMales,nbFemales,Lm,Lf,Am,Af,Mm,Mf,Gf";

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
		m_sunsetOffset = parameters[c++].GetFloat();

		return msg;
	}

	ERMsg CSpruceBudwormDispersal::OnExecuteHourly()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		//m_output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output
		CTPeriod overallPeriod;

		map<CTRef, array<CStatistic, NB_OUTPUTS>> flyers;

		//for all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			//CTStatMatrix annualStat(p, NB_OUTPUTS);

			CSBWStand stand(this);
			CSBWTreePtr pTree(new CSBWTree(&stand));

			//Create tree
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, NOT_INIT, false, 0));

			//Create stand
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			stand.m_sunsetOffset = m_sunsetOffset;
			stand.m_host.push_front(pTree);

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
										//size_t h° = size_t(h);

										const CWeatherDay& day¹ = day°.GetNext();
										const CWeatherDay& w = h < 24 ? day° : day¹;

										double T = budworm.get_Tair(w, h < 24 ? h : h - 24.0);
										double P = budworm.get_Prcp(w, h < 24 ? h : h - 24.0);

										bExodus = budworm.GetExodus(T, P, tau);
										if (bExodus)
										{
											size_t sex = budworm.GetSex();

											static const double SEX_RATIO[2] = { 0.3 / 0.7, 1.0 };
											CTRef TRefTmp = TRef + (size_t(h) - TRef.GetHour());
											//annualStat[TRefTmp][O_MALES_EXODUS + sex] += budworm.GetScaleFactor()*SEX_RATIO[sex];
											//annualStat[TRefTmp][(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] += 0;
											////liftoff cannot be mean by hour : create biasis hours... hten onlky take the first one
											////annualStat[TRefTmp][O_LIFTOFF_MALE + sex] += budworm.GetScaleFactor()*h;
											////if (!annualStat[TRefTmp][O_LM + sex].IsInit())
											//annualStat[TRefTmp][O_LM + sex] += (h - size_t(h))*3600;

											////if (!annualStat[TRefTmp][O_AM + sex].IsInit())
											//annualStat[TRefTmp][O_AM + sex] = budworm.GetA();
											//
											////if (!annualStat[TRefTmp][O_MM + sex].IsInit())
											//annualStat[TRefTmp][O_MM + sex] = budworm.GetM();
											//
											//if (sex == FEMALE)
											//	//if (!annualStat[TRefTmp][O_GF + sex].IsInit())
											//	annualStat[TRefTmp][O_GF] = budworm.GetG();

											//budworm.SetStatus(CIndividual::DEAD);
											//budworm.SetDeath(CIndividual::EXODUS);

											overallPeriod += TRefTmp;

											array<CStatistic, NB_OUTPUTS> tmp;
											
											tmp[O_MALES_EXODUS + sex] += budworm.GetScaleFactor()*SEX_RATIO[sex];
											tmp[(sex == MALE) ? O_FEMALES_EXODUS : O_MALES_EXODUS] += 0;
											tmp[O_LM + sex] = (h - size_t(h)) * 3600;
											tmp[O_AM + sex] = budworm.GetA();
											tmp[O_MM + sex] = budworm.GetM();
											tmp[O_GF] = (sex == FEMALE) ? budworm.GetG() : -999;
											
											if(flyers.find(TRefTmp) == flyers.end())
												flyers[TRefTmp] = tmp;
											else if (m_randomGenerator.Randu()>0.5)
												flyers[TRefTmp] = tmp;


											budworm.SetStatus(CIndividual::DEAD);
											budworm.SetDeath(CIndividual::EXODUS);



										}//if exodus occurd
									}//for t in exodus period
								}//if exodus occur
								//}//if is exodus hour
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

		//copy stat to output
		for (map<CTRef, array<CStatistic, NB_OUTPUTS>>::iterator it = flyers.begin(); it != flyers.end(); it++)
			for (size_t v = 0; v <NB_OUTPUTS; v++)
				if (it->second.at(v).IsInit())
					m_output[it->first][v] = it->second.at(v)[v <= 1 ? SUM : MEAN];

		return msg;
	}

}