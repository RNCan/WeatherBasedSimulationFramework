//*****************************************************************************
// Class: CActiaInterrupta_OBL_SBW
//          
//
// Description: Biology of ActiaInterrupta in relation with OBL and SBW
//*****************************************************************************
// 03/03/2020   Rémi Saint-Amant    Creation
//*****************************************************************************


#include "ActiaInterrupta_OBL_SBW.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::ActiaInterrupta;

namespace WBSF
{

	//*********************************************************************************
	//CActiaInterrupta_OBL_SBW class
	CActiaInterrupta_OBL_SBW::CActiaInterrupta_OBL_SBW(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor, CIndividualPtr& pAssociateHost) :
		CActiaInterrupta(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		m_pAssociateHost = pAssociateHost;
		m_broods_by_host.fill(0);
	}

	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CActiaInterrupta_OBL_SBW& CActiaInterrupta_OBL_SBW::operator=(const CActiaInterrupta_OBL_SBW& in)
	{
		if (&in != this)
		{
			CActiaInterrupta::operator=(in);
			m_pAssociateHost = in.m_pAssociateHost;
		}

		return *this;
	}

	void CActiaInterrupta_OBL_SBW::OnNewDay(const CWeatherDay& weather)
	{
		CActiaInterrupta::OnNewDay(weather);

		m_broods_by_host.fill(0);
	}

	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CActiaInterrupta_OBL_SBW::Live(const CWeatherDay& weather)
	{
		//adjust the number of host available

		CActiaInterrupta_OBL_SBW_Stand* pStand = GetStand();
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



		CActiaInterrupta::Live(weather);

		if (!m_pAssociateHost.expired() && !m_diapauseTRef.IsInit() && GetStage() == MAGGOT)
		{
			double dayLength = weather.GetDayLength() / 3600.; //in hours
			if (weather.GetTRef().GetJDay() > 173 && dayLength < GetStand()->m_criticalDaylength)
			{
				//Switch
				//Tests indicate that the best hypothesis is 2 (parasitoid enters diapause as soon as its host is induced): 
				int Hypothesis = 2;

				if (GetStage() == MAGGOT)
				{
					switch (Hypothesis)
					{
						//case 1: //test only once when ActiaInterrupta reach diapause age
						//	//if Jday greater than 173 and the ActiaInterrupta age pass through diapause age this day
						//	if (m_lastAge <= GetStand()->m_diapauseAge && m_age >= GetStand()->m_diapauseAge)
						//		//if the host is in diapause, ActiaInterrupta will enter in diapause. If not, this insect will develop until die. 
						//		if (m_pAssociateHost.lock()->IsInDiapause(weather.GetTRef()))
						//		{
						//			m_diapauseTRef = weather.GetTRef();
						//		}
						//	break;
							//do not use diapause age, only use host diapause induction
					case 2:
						//if the host is induced in diapause, ActiaInterrupta enters in diapause. 
						if (m_pAssociateHost.lock()->IsInDiapause(weather.GetTRef()))
						{
							m_diapauseTRef = weather.GetTRef();
							m_age = MAGGOT;//reset age to zero
						}
						break;
						////case 3:
						////	//if the host is in diapause, ActiaInterrupta enters in diapause. 
						////	if (m_pAssociateHost.lock()->IsInDiapause2(weather.GetTRef()))
						////	{
						////		m_diapauseTRef = weather.GetTRef();
						////	}
						//	break;
					default:ASSERT(false);
					}
				}
			}
		}
	}


	void CActiaInterrupta_OBL_SBW::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		ASSERT(m_totalBroods <= m_Pmax+1);
		
		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);
			
			CActiaInterrupta_OBL_SBW_Stand* pStand = GetStand(); ASSERT(pStand);
			CIndividualPtr pAssociateHost = pStand->SelectRandomHost(true);
			
			double attRate = pStand->m_generationAttrition;//1% of survival by default
			double scaleFactor = m_broods * m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CActiaInterrupta_OBL_SBW>(m_pHost, weather.GetTRef(), MAGGOT, FEMALE, true, m_generation + 1, scaleFactor, pAssociateHost);
			m_pHost->push_front(object);

			assert(object->get_property("HostType") == "0" || object->get_property("HostType") == "1");
			size_t hostType = stoi(object->get_property("HostType"));
			m_broods_by_host[hostType] += m_broods;
		}
	}

	// kills when host die
	// Output:  Individual's state is updated to follow update
	void CActiaInterrupta_OBL_SBW::Die(const CWeatherDay& weather)
	{
		CActiaInterrupta::Die(weather);
		if (!m_pAssociateHost.expired())
		{
			//if the associate host die, the parasite also die
			if (GetStage() == MAGGOT && !m_pAssociateHost.lock()->IsAlive())
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
	void CActiaInterrupta_OBL_SBW::GetStat(CTRef d, CModelStat& stat)
	{
		CActiaInterrupta::GetStat(d, stat);

		if (IsCreated(d))
		{
			//assert(get_property("HostType") == "0" || get_property("HostType") == "1");
			//size_t hostType = stoi(get_property("HostType"));

			stat[S_BROOD_OBL] += m_broods_by_host[H_OBL] * m_scaleFactor;
			stat[M_BROOD_OBL] = stat[S_BROOD_OBL];
			stat[S_BROOD_SBW] += m_broods_by_host[H_SBW] * m_scaleFactor;
			stat[M_BROOD_SBW] = stat[S_BROOD_SBW];

			if (m_death == HOST_DIE)
				stat[S_HOST_DIE] += m_scaleFactor;

			if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == HOST_DIE)
				stat[M_HOST_DIE] += m_scaleFactor;
		}

	}

	std::string CActiaInterrupta_OBL_SBW::get_property(const std::string& name)const
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

	//bool CActiaInterrupta_OBL_SBW::CanPack(const CIndividualPtr& in)const
	//{
	//	CActiaInterrupta_OBL_SBW* pIn = static_cast<CActiaInterrupta_OBL_SBW*>(in.get());
	//	return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_bDiapause == m_bDiapause;
	//}

	//void CActiaInterrupta_OBL_SBW::Pack(const CIndividualPtr& pBug)
	//{
	//	CActiaInterrupta_OBL_SBW* in = (CActiaInterrupta_OBL_SBW*)pBug.get();

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

	void CActiaInterrupta_OBL_SBW_Host::Initialize(const CInitialPopulation& initValue)
	{
		clear();

		CActiaInterrupta_OBL_SBW_Stand* pStand = GetStand();
		const std::shared_ptr<WBSF::CHost>& pHost = pStand->m_OBLStand.m_host.front();
		WBSF::CHost::const_iterator itOBL = pHost->begin();
		for (CInitialPopulation::const_iterator it = initValue.begin(); it != initValue.end(); it++)
		{
			CIndividualPtr pOBL = *itOBL;
			push_back(std::make_shared<CActiaInterrupta_OBL_SBW>(this, it->m_creationDate, it->m_age, it->m_sex, it->m_bFertil, it->m_generation, it->m_scaleFactor, pOBL));
			m_initialPopulation += it->m_scaleFactor;

			//loop over host
			itOBL++;
			if (itOBL == pHost->end())
				itOBL = pHost->begin();
		}
	}

	//std::string CActiaInterrupta_OBL_SBW_Host::get_property(const std::string& name)
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

	void CActiaInterrupta_OBL_SBW_Stand::Live(const CWeatherDay& weather)
	{
		CActiaInterruptaStand::Live(weather);
		m_OBLStand.Live(weather);
		m_SBWStand.Live(weather);
	}

	void CActiaInterrupta_OBL_SBW_Stand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CActiaInterruptaStand::GetStat(d, stat, generation);


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
	bool CActiaInterrupta_OBL_SBW_Stand::AdjustPopulation()
	{
		bool bAdjuste = CActiaInterruptaStand::AdjustPopulation();


		if (m_OBLStand.AdjustPopulation())
		{
			//reselect bad ptr
			const std::shared_ptr<WBSF::CHost>& pObjects = m_host.front();
			for (CIndividualPtrContainer::iterator it = pObjects->begin(); it != pObjects->end(); it++)
			{
				CIndividual* pInd = it->get();
				ASSERT(pInd != NULL);
				CActiaInterrupta_OBL_SBW* pActiaInterrupta = static_cast<CActiaInterrupta_OBL_SBW*>(pInd);
				ASSERT(pActiaInterrupta);

				if (pActiaInterrupta->m_pAssociateHost.expired())
				{
					//this ActiaInterrupta don't have host anymore
					//select a new one
					pActiaInterrupta->m_pAssociateHost = SelectRandomHost(false);
				}
			}
		}

		return bAdjuste;
	}
	size_t CActiaInterrupta_OBL_SBW_Stand::GetNbObjectAlive()const
	{
		return
			CActiaInterruptaStand::GetNbObjectAlive() +
			m_OBLStand.GetNbObjectAlive() +
			m_SBWStand.GetNbObjectAlive();
	}
	void CActiaInterrupta_OBL_SBW_Stand::HappyNewYear()
	{
		CActiaInterruptaStand::HappyNewYear();
		m_OBLStand.HappyNewYear();
		m_SBWStand.HappyNewYear();
	}
	double CActiaInterrupta_OBL_SBW_Stand::GetAI(bool bIncludeLast)const
	{
		return CActiaInterruptaStand::GetAI(bIncludeLast);
	}

	CHostPtr CActiaInterrupta_OBL_SBW_Stand::GetNearestHost(CHost* pHost)
	{
		return CActiaInterruptaStand::GetNearestHost(pHost);
		//m_OBLStand.GetNearestHost(NULL);
		//m_SBWStand.GetNearestHost(NULL);
	}

	CIndividualPtr CActiaInterrupta_OBL_SBW_Stand::SelectRandomHost(bool bUseSBW)
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
