﻿//*****************************************************************************
// Class: CTranosema_OBL_SBW
//          
//
// Description: Biology of Tranosema rostrale in relation with OBL and SBW
//*****************************************************************************
// 21/11/2017   Rémi Saint-Amant    Creation
//*****************************************************************************


#include "Tranosema_OBL_SBW.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::Tranosema;

namespace WBSF
{

	//*********************************************************************************
	//CTranosema_OBL_SBW class
	CTranosema_OBL_SBW::CTranosema_OBL_SBW(CHost* pHost, CTRef creationDate, double age, WBSF::TSex sex, bool bFertil, size_t generation, double scaleFactor, CIndividualPtr& pAssociateHost) :
		CTranosema(pHost, creationDate, age, sex, bFertil, generation, scaleFactor) 
	{
		m_pAssociateHost = pAssociateHost;
	}

	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CTranosema_OBL_SBW& CTranosema_OBL_SBW::operator=(const CTranosema_OBL_SBW& in)
	{
		if (&in != this)
		{
			CTranosema::operator=(in);
			m_pAssociateHost = in.m_pAssociateHost;
		}

		return *this;
	}

	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CTranosema_OBL_SBW::Live(const CWeatherDay& weather)
	{
		//adjust the number of host available
		
		CTranosema_OBL_SBW_Stand* pStand = GetStand();
		ASSERT(pStand->m_OBLStand.m_host.size() == 1);
		ASSERT(pStand->m_SBWStand.m_host.size() == 1);
		
		
		if (m_sex == FEMALE && m_age >= ADULT)
		{
			//m_Nh is only update for female
			
			double nbAttackable = 0;

			const std::shared_ptr<WBSF::CHost>& pOBLObjects = pStand->m_OBLStand.m_host.front();
			for (auto it = pOBLObjects->begin(); it != pOBLObjects->end(); it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= OBL::L1 && stage <= OBL::L6 && stage != OBL::L3D)
				{
					if ((*it)->IsAlive())
						nbAttackable += (*it)->GetScaleFactor();
				}
			}

			const std::shared_ptr<WBSF::CHost>& pSBWObjects = pStand->m_SBWStand.m_host.front();
			for (auto it = pSBWObjects->begin(); it != pSBWObjects->end(); it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= SBW::L2 && stage <= SBW::L6)
				{
					if ((*it)->IsAlive())
						nbAttackable += (*it)->GetScaleFactor();
				}

			}

			m_Nh = nbAttackable;
		}

		CTranosema::Live(weather);

		if (!m_pAssociateHost.expired() && !m_diapauseTRef.IsInit())
		{
			double dayLength = weather.GetDayLength() / 3600.; //in hours
			if (weather.GetTRef().GetJDay() > 173 && dayLength < GetStand()->m_criticalDaylength)
			{
				//Switch
				//Tests indicate that the best hypothesis is 2 (parasitoid enters diapause as soon as its host is induced): 
				int Hypothesis = 2;

				if (GetStage() == EGG)
				{
					switch (Hypothesis) {
					case 1: //test only once when tranosema reach diapause age
						//if Jday greather than 173 and the tranosema age pass through diapause age this day
						if (m_lastAge <= GetStand()->m_diapauseAge && m_age >= GetStand()->m_diapauseAge)
							//if the host is in diapause, tranosema will enter in diapause. If not, this insect will develop until die. 
							if (m_pAssociateHost.lock()->IsInDiapause(weather.GetTRef()))
							{
								m_diapauseTRef = weather.GetTRef();
							}
						break;
						//do not use diapause age, only use host diapause induction
					case 2:
						//if the host is induced in diapause, tranosema enters in diapause. 
						if (m_pAssociateHost.lock()->IsInDiapause(weather.GetTRef()))
						{
							m_diapauseTRef = weather.GetTRef();
						}
						break;
					//case 3:
					//	//if the host is in diapause, tranosema enters in diapause. 
					//	if (m_pAssociateHost.lock()->IsInDiapause2(weather.GetTRef()))
					//	{
					//		m_diapauseTRef = weather.GetTRef();
					//	}
					//	break;
					}
				}
			}
		}
	}


	void CTranosema_OBL_SBW::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		ASSERT(m_totalBroods <= m_Pmax+1);

		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);
			CTranosema_OBL_SBW_Stand* pStand = GetStand(); ASSERT(pStand);
			CIndividualPtr pAssociateHost  = pStand->SelectRandomHost(true);

			double attRate = GetStand()->m_bApplyAttrition ? pStand->m_generationAttrition : 1;//10% of survival by default
			double scaleFactor = m_broods*m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CTranosema_OBL_SBW>(m_pHost, weather.GetTRef(), EGG, FEMALE, true, m_generation + 1, scaleFactor, pAssociateHost);
			m_pHost->push_front(object);
		}
	}

	// kills when host die
	// Output:  Individual's state is updated to follow update
	void CTranosema_OBL_SBW::Die(const CWeatherDay& weather)
	{
		CTranosema::Die(weather);
		if (!m_pAssociateHost.expired())
		{
			//if the associat host die, the parazite also die
			if (GetStage() == EGG && !m_pAssociateHost.lock()->IsAlive())
			{
				m_status = DEAD;
				m_death = HOST_DIE;
			}
		}

	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CTranosema_OBL_SBW::GetStat(CTRef d, CModelStat& stat)
	{
		CTranosema::GetStat(d, stat);

		if (IsCreated(d))
		{
			

			if (m_death == HOST_DIE)
				stat[S_HOST_DIE] += m_scaleFactor;



			if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == HOST_DIE)
				stat[E_HOST_DIE] += m_scaleFactor;
		}

	}

	//bool CTranosema_OBL_SBW::CanPack(const CIndividualPtr& in)const
	//{
	//	CTranosema_OBL_SBW* pIn = static_cast<CTranosema_OBL_SBW*>(in.get());
	//	return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_bDiapause == m_bDiapause;
	//}

	//void CTranosema_OBL_SBW::Pack(const CIndividualPtr& pBug)
	//{
	//	CTranosema_OBL_SBW* in = (CTranosema_OBL_SBW*)pBug.get();

	//	m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

	//	CIndividual::Pack(pBug);
	//}

	//*********************************************************************************************************************
	//Host

	void CTranosema_OBL_SBW_Host::Initialize(const CInitialPopulation& initValue)
	{
		clear();

		CTranosema_OBL_SBW_Stand* pStand = GetStand();
		const std::shared_ptr<WBSF::CHost>& pOBLObjects = pStand->m_OBLStand.m_host.front();
		WBSF::CHost::const_iterator itOBL = pOBLObjects->begin();
		for (CInitialPopulation::const_iterator it = initValue.begin(); it != initValue.end(); it++)
		{
			CIndividualPtr pOBL = *itOBL;
			push_back(std::make_shared<CTranosema_OBL_SBW>(this, it->m_creationDate, it->m_age, it->m_sex, it->m_bFertil, it->m_generation, it->m_scaleFactor, pOBL));
			m_initialPopulation += it->m_scaleFactor;

			//loop over OBL object
			itOBL++;
			if (itOBL == pOBLObjects->end())
				itOBL = pOBLObjects->begin();
		}
	}
	//*********************************************************************************************************************
	//Stand

	void CTranosema_OBL_SBW_Stand::Live(const CWeatherDay& weather)
	{
		CTranosemaStand::Live(weather);
		m_OBLStand.Live(weather);
		m_SBWStand.Live(weather);
	}

	void CTranosema_OBL_SBW_Stand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CTranosemaStand::GetStat(d, stat, generation);

		
		ASSERT(m_OBLStand.m_host.size() == 1);
		ASSERT(m_SBWStand.m_host.size() == 1);

		const std::shared_ptr<WBSF::CHost>& pOBLObjects = m_OBLStand.m_host.front();
		for (auto it = pOBLObjects->begin(); it != pOBLObjects->end(); it++)
		{
			size_t stage = (*it)->GetStage();
			if (stage >= OBL::L1 && stage <= OBL::L6 && stage != OBL::L3D)
			{
				if ((*it)->IsAlive())
					stat[S_NB_OBL] += (*it)->GetScaleFactor();
			}

			if (stage == OBL::L3D)
				stat[S_NB_OBL_L3D] += (*it)->GetScaleFactor();
		}

		const std::shared_ptr<WBSF::CHost>& pSBWObjects = m_SBWStand.m_host.front();
		for (auto it = pSBWObjects->begin(); it != pSBWObjects->end(); it++)
		{
			size_t stage = (*it)->GetStage();
			if (stage >= SBW::L2 && stage <= SBW::L6)
			{
				if ((*it)->IsAlive())
					stat[S_NB_SBW] += (*it)->GetScaleFactor();
			}
		}

		/*CModelStat statOBL;
		m_OBLStand.GetStat(d, statOBL, -1);
		nbAttacka
		CModelStat statSBW;
		m_SBWStand.GetStat(d, statSBW, -1);*/
	}
	bool CTranosema_OBL_SBW_Stand::AdjustPopulation()
	{
		bool bAdjuste = CTranosemaStand::AdjustPopulation();
		
		
		if (m_OBLStand.AdjustPopulation())
		{
			//reselect bad ptr
			const std::shared_ptr<WBSF::CHost>& pObjects = m_host.front();
			for (CIndividualPtrContainer::iterator it = pObjects->begin(); it != pObjects->end(); it++)
			{
				CIndividual* pInd = it->get();
				ASSERT(pInd != NULL);
				CTranosema_OBL_SBW* pTranosema = static_cast<CTranosema_OBL_SBW*>(pInd);
				ASSERT(pTranosema);

				if (pTranosema->m_pAssociateHost.expired())
				{
					//this tranosema don't have host anymore
					//select a new one
					pTranosema->m_pAssociateHost = SelectRandomHost(false);
				}
			}
		}

		return bAdjuste;
	}
	size_t CTranosema_OBL_SBW_Stand::GetNbObjectAlive()const
	{
		return	CTranosemaStand::GetNbObjectAlive();
				//m_OBLStand.GetNbObjectAlive() +
				//m_SBWStand.GetNbObjectAlive();
	}
	void CTranosema_OBL_SBW_Stand::HappyNewYear()
	{
		CTranosemaStand::HappyNewYear();
		m_OBLStand.HappyNewYear();
		m_SBWStand.HappyNewYear();
	}
	double CTranosema_OBL_SBW_Stand::GetAI(bool bIncludeLast)const
	{
		return CTranosemaStand::GetAI(bIncludeLast);
		//m_OBLStand.Live(weather);
		//m_SBWStand.Live(weather);
	}

	CHostPtr CTranosema_OBL_SBW_Stand::GetNearestHost(CHost* pHost)
	{
		return CTranosemaStand::GetNearestHost(pHost);
		//m_OBLStand.GetNearestHost(NULL);
		//m_SBWStand.GetNearestHost(NULL);
	}

	CIndividualPtr CTranosema_OBL_SBW_Stand::SelectRandomHost(bool bUseSBW)
	{
		CIndividualPtr pHost;
		//std::weak_ptr<CIndividual> pHost;

		const std::shared_ptr<WBSF::CHost>& pOBLObjects = m_OBLStand.m_host.front();
		ASSERT(!pOBLObjects->empty());
		const std::shared_ptr<WBSF::CHost>& pSBWObjects = m_SBWStand.m_host.front();
		ASSERT(!pSBWObjects->empty());


		double nbAttackable = 0;
		for (auto it = pOBLObjects->begin(); it != pOBLObjects->end(); it++)
		{
			size_t stage = (*it)->GetStage();
			if (stage >= OBL::L1 && stage <= OBL::L6 && stage != OBL::L3D)
			{
				if ((*it)->IsAlive())
					nbAttackable += (*it)->GetScaleFactor();
			}

		}
		
		if (bUseSBW)
		{
			for (auto it = pSBWObjects->begin(); it != pSBWObjects->end(); it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= SBW::L2 && stage <= SBW::L6)
				{
					if ((*it)->IsAlive())
						nbAttackable += (*it)->GetScaleFactor();
				}
			}
		}
		
		double rand = RandomGenerator().Rand(0.0, nbAttackable);
		//select between OBL and SBW
		nbAttackable = 0;
		for (auto it = pOBLObjects->begin(); it != pOBLObjects->end() && pHost.get()==NULL; it++)
		{
			size_t stage = (*it)->GetStage();
			if (stage >= OBL::L1 && stage <= OBL::L6 && stage != OBL::L3D)
			{
				if ((*it)->IsAlive())
					nbAttackable += (*it)->GetScaleFactor();
			}

			if (nbAttackable >= rand)
				pHost = *it;
		}

		if (bUseSBW)
		{
			for (auto it = pSBWObjects->begin(); it != pSBWObjects->end() && pHost.get() == NULL; it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= SBW::L2 && stage <= SBW::L6)
				{
					if ((*it)->IsAlive())
						nbAttackable += (*it)->GetScaleFactor();
				}

				if (nbAttackable >= rand)
					pHost = *it;
			}
		}

		ASSERT(pHost);
		return pHost;
	}
}




