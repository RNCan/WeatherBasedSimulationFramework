﻿//*****************************************************************************
// Class: CMeteorusTrachynotus_OBL_SBW
//          
//
// Description: Biology of MeteorusTrachynotus in relation with OBL and SBW
//*****************************************************************************
// 03/03/2020   Rémi Saint-Amant    Creation
//*****************************************************************************


#include "MeteorusTrachynotus_OBL_SBW.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::MeteorusTrachynotus;

namespace WBSF
{

	//*********************************************************************************
	//CMeteorusTrachynotus_OBL_SBW class
	CMeteorusTrachynotus_OBL_SBW::CMeteorusTrachynotus_OBL_SBW(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor, CIndividualPtr& pAssociateHost) :
		CMeteorusTrachynotus(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		m_pAssociateHost = pAssociateHost;
		m_Nh = 0;
		m_nb_attacks_by_host.fill(0);
		m_broods_by_host.fill(0);
	}

	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CMeteorusTrachynotus_OBL_SBW& CMeteorusTrachynotus_OBL_SBW::operator=(const CMeteorusTrachynotus_OBL_SBW& in)
	{
		if (&in != this)
		{
			CMeteorusTrachynotus::operator=(in);
			m_pAssociateHost = in.m_pAssociateHost;
		}

		return *this;
	}

	void CMeteorusTrachynotus_OBL_SBW::OnNewDay(const CWeatherDay& weather)
	{
		CMeteorusTrachynotus::OnNewDay(weather);

		m_Nh = 0;
		m_nb_attacks_by_host.fill(0);
		m_broods_by_host.fill(0);
	}

	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CMeteorusTrachynotus_OBL_SBW::Live(const CWeatherDay& weather)
	{
		//adjust the number of host available

		CMeteorusTrachynotus_OBL_SBW_Stand* pStand = GetStand();
		ASSERT(pStand->m_OBLStand.m_host.size() == 1);
		ASSERT(pStand->m_SBWStand.m_host.size() == 1);


		if (m_sex == FEMALE && m_age >= ADULT && GetStageAge() >= GetStand()->m_preOvip)
		{
			//m_Nh is only update for female
			const std::shared_ptr<WBSF::CHost>& pOBLObjects = pStand->m_OBLStand.m_host.front();
			for (auto it = pOBLObjects->begin(); it != pOBLObjects->end(); it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= OBL::L1 && stage <= OBL::L6 && stage != OBL::L3D)
				{
					if ((*it)->IsAlive())
						m_nb_attacks_by_host[H_OBL] += (*it)->GetScaleFactor();
				}
			}

			const std::shared_ptr<WBSF::CHost>& pSBWObjects = pStand->m_SBWStand.m_host.front();
			for (auto it = pSBWObjects->begin(); it != pSBWObjects->end(); it++)
			{
				size_t stage = (*it)->GetStage();
				if (stage >= SBW::L2 && stage <= SBW::L6)
				{
					if ((*it)->IsAlive())
						m_nb_attacks_by_host[H_SBW] += (*it)->GetScaleFactor();
				}

			}


			m_Nh = m_nb_attacks_by_host[H_OBL] + m_nb_attacks_by_host[H_SBW];
		}



		CMeteorusTrachynotus::Live(weather);


		if (!m_pAssociateHost.expired() && 
			!m_diapauseTRef.IsInit() && 
			GetGeneration()!=0 &&
			( GetStage() == EGG) )//unclear what ot do here???
		{
			double dayLength = weather.GetDayLength() / 3600.; //in hours
			if (weather.GetTRef().GetJDay() > 173 && dayLength < GetStand()->m_criticalDaylength)
			{
				//if the host is induced in diapause, MeteorusTrachynotus enters in diapause. 
				if (m_pAssociateHost.lock()->IsInDiapause(weather.GetTRef()))
				{
					m_diapauseTRef = weather.GetTRef();
					m_age = 0;//reset age to zero
				}

			}
		}
	}


	void CMeteorusTrachynotus_OBL_SBW::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		ASSERT(m_totalBroods <= m_Pmax + 1);

		CMeteorusTrachynotus::Brood(weather);


		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);

			CMeteorusTrachynotus_OBL_SBW_Stand* pStand = GetStand(); ASSERT(pStand);
			CIndividualPtr pAssociateHost = pStand->SelectRandomHost(true);

			double attRate = pStand->m_generationAttrition;//1% of survival by default
			double scaleFactor = m_broods * m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CMeteorusTrachynotus_OBL_SBW>(m_pHost, weather.GetTRef(), EGG, FEMALE, true, m_generation + 1, scaleFactor, pAssociateHost);
			m_pHost->push_front(object);

			assert(object->get_property("HostType") == "0" || object->get_property("HostType") == "1");
			size_t hostType = stoi(object->get_property("HostType"));
			m_broods_by_host[hostType] += m_broods;
		}
	}

	// kills when host die
	// Output:  Individual's state is updated to follow update
	void CMeteorusTrachynotus_OBL_SBW::Die(const CWeatherDay& weather)
	{
		CMeteorusTrachynotus::Die(weather);
		if (!m_pAssociateHost.expired())
		{
			//if the associate host die, the parasite also die
			if ((GetStage() == EGG || GetStage() == LARVA) &&
				!m_pAssociateHost.lock()->IsAlive())
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
	void CMeteorusTrachynotus_OBL_SBW::GetStat(CTRef d, CModelStat& stat)
	{
		CMeteorusTrachynotus::GetStat(d, stat);

		if (IsCreated(d))
		{
			//assert(get_property("HostType") == "0" || get_property("HostType") == "1");
			//size_t hostType = stoi(get_property("HostType"));

			stat[S_BROODS_OBL] += m_broods_by_host[H_OBL] * m_scaleFactor;
			stat[S_BROODS_SBW] += m_broods_by_host[H_SBW] * m_scaleFactor;

			//			stat[S_ATTACKS_OBL] += m_nb_attacks_by_host[H_OBL] * m_scaleFactor;
				//		stat[S_ATTACKS_SBW] += m_nb_attacks_by_host[H_SBW] * m_scaleFactor;


			if (m_death == HOST_DIE)
				stat[S_HOST_DIE] += m_scaleFactor;

			if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == HOST_DIE)
				stat[M_HOST_DIE] += m_scaleFactor;
		}

	}

	std::string CMeteorusTrachynotus_OBL_SBW::get_property(const std::string& name)const
	{
		std::string prop;
		if (!m_pAssociateHost.expired())
		{
			if (name == "HostType")
			{
				string test = typeid(*m_pAssociateHost.lock()).name();
				if (test == typeid(CObliqueBandedLeafroller).name())
					prop = to_string(H_OBL);
				else if (test == typeid(CSpruceBudworm).name())
					prop = to_string(H_SBW);
				//prop = m_pAssociateHost.lock()->get_property(name);
			}

		}

		return prop;
	}

	//bool CMeteorusTrachynotus_OBL_SBW::CanPack(const CIndividualPtr& in)const
	//{
	//	CMeteorusTrachynotus_OBL_SBW* pIn = static_cast<CMeteorusTrachynotus_OBL_SBW*>(in.get());
	//	return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_bDiapause == m_bDiapause;
	//}

	//void CMeteorusTrachynotus_OBL_SBW::Pack(const CIndividualPtr& pBug)
	//{
	//	CMeteorusTrachynotus_OBL_SBW* in = (CMeteorusTrachynotus_OBL_SBW*)pBug.get();

	//	m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

	//	CIndividual::Pack(pBug);
	//}

	//std::string CObliqueBandedLeafrollerEx::get_property(const std::string& name)
	//{
	//	std::string prop;
	//	if (name == "HostType")
	//		prop = to_string(H_OBL);//OBL by default, need override to change that

	//	return prop;
	//}

	//std::string CSpruceBudwormEx::get_property(const std::string& name)
	//{
	//	std::string prop;
	//	if (name == "HostType")
	//		prop = to_string(H_SBW);//OBL by default, need override to change that

	//	return prop;
	//}

	//*********************************************************************************************************************
	//Host

	void CMeteorusTrachynotus_OBL_SBW_Host::Initialize(const CInitialPopulation& initValue)
	{
		clear();

		CMeteorusTrachynotus_OBL_SBW_Stand* pStand = GetStand();
		const std::shared_ptr<WBSF::CHost>& pHost = pStand->m_OBLStand.m_host.front();
		WBSF::CHost::const_iterator itOBL = pHost->begin();
		for (CInitialPopulation::const_iterator it = initValue.begin(); it != initValue.end(); it++)
		{
			CIndividualPtr pOBL = *itOBL;
			push_back(std::make_shared<CMeteorusTrachynotus_OBL_SBW>(this, it->m_creationDate, it->m_age, it->m_sex, it->m_bFertil, it->m_generation, it->m_scaleFactor, pOBL));
			m_initialPopulation += it->m_scaleFactor;

			//loop over host
			itOBL++;
			if (itOBL == pHost->end())
				itOBL = pHost->begin();
		}
	}

	//std::string CMeteorusTrachynotus_OBL_SBW_Host::get_property(const std::string& name)
	//{
	//	std::string prop;
	//	if (name == "HostType")
	//	{
	//		/*string class_name = typeid(begin()).name();
	//		std::type_info(begin());
	//		if (class_name == "CObliqueBandedLeafroller")
	//			prop = to_string(0);
	//		else
	//			prop = to_string(1);*/
	//	}


	//	return prop;
	//}
	//*********************************************************************************************************************
	//Stand

	void CMeteorusTrachynotus_OBL_SBW_Stand::Live(const CWeatherDay& weather)
	{
		CMeteorusTrachynotusStand::Live(weather);
		m_OBLStand.Live(weather);
		m_SBWStand.Live(weather);
	}

	void CMeteorusTrachynotus_OBL_SBW_Stand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CMeteorusTrachynotusStand::GetStat(d, stat, generation);


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
	bool CMeteorusTrachynotus_OBL_SBW_Stand::AdjustPopulation()
	{
		bool bAdjuste = CMeteorusTrachynotusStand::AdjustPopulation();


		if (m_OBLStand.AdjustPopulation())
		{
			//reselect bad ptr
			const std::shared_ptr<WBSF::CHost>& pObjects = m_host.front();
			for (CIndividualPtrContainer::iterator it = pObjects->begin(); it != pObjects->end(); it++)
			{
				CIndividual* pInd = it->get();
				ASSERT(pInd != NULL);
				CMeteorusTrachynotus_OBL_SBW* pMeteorusTrachynotus = static_cast<CMeteorusTrachynotus_OBL_SBW*>(pInd);
				ASSERT(pMeteorusTrachynotus);

				if (pMeteorusTrachynotus->m_pAssociateHost.expired())
				{
					//this MeteorusTrachynotus don't have host anymore
					//select a new one
					pMeteorusTrachynotus->m_pAssociateHost = SelectRandomHost(false);
				}
			}
		}

		return bAdjuste;
	}
	size_t CMeteorusTrachynotus_OBL_SBW_Stand::GetNbObjectAlive()const
	{
		return
			CMeteorusTrachynotusStand::GetNbObjectAlive() +
			m_OBLStand.GetNbObjectAlive() +
			m_SBWStand.GetNbObjectAlive();
	}
	void CMeteorusTrachynotus_OBL_SBW_Stand::HappyNewYear()
	{
		CMeteorusTrachynotusStand::HappyNewYear();
		m_OBLStand.HappyNewYear();
		m_SBWStand.HappyNewYear();
	}
	double CMeteorusTrachynotus_OBL_SBW_Stand::GetAI(bool bIncludeLast)const
	{
		return CMeteorusTrachynotusStand::GetAI(bIncludeLast);
	}

	CHostPtr CMeteorusTrachynotus_OBL_SBW_Stand::GetNearestHost(CHost* pHost)
	{
		return CMeteorusTrachynotusStand::GetNearestHost(pHost);
		//m_OBLStand.GetNearestHost(NULL);
		//m_SBWStand.GetNearestHost(NULL);
	}

	CIndividualPtr CMeteorusTrachynotus_OBL_SBW_Stand::SelectRandomHost(bool bUseSBW)
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
		for (auto it = pOBLObjects->begin(); it != pOBLObjects->end() && pHost.get() == NULL; it++)
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