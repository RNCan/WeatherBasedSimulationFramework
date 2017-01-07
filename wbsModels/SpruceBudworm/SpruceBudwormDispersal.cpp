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

namespace WBSF
{
	using namespace SBW;

	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBudwormDispersal::CreateObject);

	enum Toutput { O_NB_MALES, O_NB_FEMALES, O_AM, O_AF, O_MM, O_MF, O_GF, NB_OUTPUTS };
	extern char HOURLY_HEADER[] = "nbMales,nbFemales,Am,Af,Mm,Mf,Gf";

	CSpruceBudwormDispersal::CSpruceBudwormDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 1;
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

		return msg;
	}

	ERMsg CSpruceBudwormDispersal::OnExecuteHourly()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		m_output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output
		

		//for all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			CTStatMatrix annualStat(p, NB_OUTPUTS);

			CSBWStand stand(this);
			CSBWTreePtr pTree(new CSBWTree(&stand));

			//Create tree
			//pTree->m_kind = m_treeKind;
			//pTree->m_nbMinObjects = 1;
			//pTree->m_nbMaxObjects = 100000;
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, NOT_INIT, false, 0));

			//Create stand
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			//stand.m_bStopL22 = true;
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


							//compute flight activity
							if (budworm.GetStage() == ADULT)
							{

								size_t h = w.GetTRef().GetHour();

								if ((*it)->GetSex() == FEMALE && h == 12)
									(*it)->Brood(day°);

								if (h < 2 || h >= 16)
								{

									__int64 t° = 0;
									__int64 tᴹ = 0;

									if (budworm.get_t(day°, t°, tᴹ))
									{
										//calculate tᶜ
										__int64 tᶜ = (t° + tᴹ) / 2;


										//now compute tau, p and flight
										if (h < 2)
											h += 24;

										__int64 t = h * 3600;
										if (t >= t° && t <= tᴹ)//petite perte ici????
										{
											double tau = double(t - tᶜ) / (tᴹ - tᶜ);

											const CWeatherDay& day¹ = day°.GetNext();
											const CWeatherDay& w = h < 24 ? day° : day¹;
											if (h >= 24)
												h -= 24;

											double T = budworm.get_Tair(w, h);
											double P = budworm.get_Prcp(w, h);

											bool bExodus = budworm.GetExodus(T, P, tau);
											if (bExodus)
											{
												size_t sex = budworm.GetSex();
												
												annualStat[TRef][O_NB_MALES + sex] += budworm.GetScaleFactor();
												annualStat[TRef][(sex == MALE)?O_NB_FEMALES : O_NB_MALES] += 0;
												annualStat[TRef][O_AM + sex] += budworm.GetScaleFactor()*budworm.GetA();
												annualStat[TRef][O_MM + sex] += budworm.GetScaleFactor()*budworm.GetM();
												if (sex==FEMALE)
													annualStat[TRef][O_GF] += budworm.GetScaleFactor()*budworm.GetG();

												budworm.SetStatus(CIndividual::DEAD);
												budworm.SetDeath(CIndividual::EXODUS);
											}
										}//if t is inside exodus period
									}//if exodus occur
								}//if is exodus hour
							}//if adult

							budworm.Die(day°);
						}//temperature is over -10 °C
					}//if is alive
				}//for all insect 
				
				HxGridTestConnection();
			}//for all hours of a year

			//copy stat to output
			for (size_t h = 0; h < annualStat.size(); h++)
				for (size_t v = 0; v <NB_OUTPUTS; v++)
					if (annualStat[h][v].IsInit())
						m_output[h][v] = annualStat[h][v][v<=1?SUM:MEAN];
		}//for all years

		

		return msg;
	}

}