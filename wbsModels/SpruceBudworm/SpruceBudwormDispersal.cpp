﻿//*****************************************************************************
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


namespace WBSF
{
	using namespace SBW;

	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBudwormDispersal::CreateObject);


	extern char HOURLY_HEADER[] = "Sex,Am,Mm,Af,Mf,Gf";
	//typedef CModelStatVectorTemplate<NB_STATS, DAILY_HEADER> CDailyOutput;

	CSpruceBudwormDispersal::CSpruceBudwormDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 0;
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

		//m_bHaveAttrition = parameters[c++].GetBool();
		//m_bFertility = parameters[c++].GetBool();
		//m_treeKind = parameters[c++].GetInt();

		//m_fixDate = parameters[c++].GetTRef();
		//m_fixAI = parameters[c++].GetReal();

		return msg;
	}

	ERMsg CSpruceBudwormDispersal::OnExecuteHourly()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));


			CSBWStand stand(this);
			CSBWTreePtr pTree(new CSBWTree(&stand));

			//Create tree
			//pTree->m_kind = m_treeKind;
			pTree->m_nbMinObjects = 200;
			pTree->m_nbMaxObjects = 2000;
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 800, 800, L2o, NOT_INIT, false, 0));

			//Create stand
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			//stand.m_bStopL22 = true;
			stand.m_host.push_front(pTree);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
//				stand.Live(m_weather.GetHour(TRef));
				
				for (CSBWTree::iterator it = pTree->begin(); it != pTree->end(); it++)
				{
					CSpruceBudworm& budworm = static_cast<CSpruceBudworm&>(*(*it).get());
					if (budworm.IsAlive())
					{
						const CHourlyData& w = m_weather.GetHour(TRef);


						budworm.Live(w, 1);

						//(*it)->Die(weather);


						//compute flight activity
						if (budworm.GetStage() == ADULT)
						{
							ASSERT(w.GetParent());
							const CWeatherDay& day° = (const CWeatherDay&)*w.GetParent();

							if ((*it)->GetSex() == FEMALE)
								(*it)->Brood(day°);


							__int64 t° = 0;
							__int64 tᴹ = 0;

							if (budworm.get_t(day°, t°, tᴹ))
							{
								//calculate tᶜ
								__int64 tᶜ = (t° + tᴹ) / 2;

								size_t h = w.GetTRef().GetHour();

								//now compute tau, p and flight
								if (h < 2)
									h += 24;

								__int64 t = h * 3600;
								if (t >= t° && t <= tᴹ)//petite perte ici????
								{
									double tau = double(t - tᶜ) / (tᴹ - tᶜ);

									const CWeatherDay& day¹ = day°.GetNext();
									const CHourlyData& w = h < 24 ? day°[h] : day¹[h - 24];

									double flightActivity = budworm.GetFlightActivity(w, tau);
									if (flightActivity)
									{
										budworm.SetStatus(CIndividual::DEAD);
										budworm.SetDeath(CIndividual::EXODUS);
									}
								}
							}
						}
					}
				}
				
				HxGridTestConnection();
			}
		}

		//This is where the model is actually executed
		CModelStatVector stat(m_weather.GetEntireTPeriod(), NB_STATS, 0, HOURLY_HEADER);//hourly output


		return msg;
	}

}