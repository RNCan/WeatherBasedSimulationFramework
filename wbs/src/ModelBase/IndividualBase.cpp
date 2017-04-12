#include "stdafx.h"
#include "IndividualBase.h"

using namespace std;

namespace WBSF
{


	//bool CIndividual::m_bUseAttrition = false;

	//*****************************************************************************
	// Object creator 
	//
	// Input: int creationDate: the day of the creation
	//		  int stage: the stage of the insect when the object is created
	//        
	//
	// Output: 
	//
	// Note: m_relativeDevRate member is modified.
	//*****************************************************************************
	CIndividual::CIndividual(CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor)
	{
		ASSERT(pHost);//host must be define at the specimen creation

		m_pHost = pHost;

		//A creation date is assigned to each individual
		m_creationDate = creationDate;

		// Stage at creation (initial generation starts as L2, other starts as eggs)
		m_age = age;
		m_lastAge = -1;

		m_status = HEALTHY;
		m_lastStatus = size_t(-1);//undifine for the moment
		m_death = NOT_DEATH;

		if (sex == NOT_INIT)
			m_sex = RandomGenerator().Rand(0, 1);
		else
			m_sex = sex;

		m_bFertil = bFertil;
		m_broods = 0;
		m_totalBroods = 0;

		m_generation = generation;
		m_scaleFactor = scaleFactor;
	}

	// Object destructor
	CIndividual::~CIndividual(void)
	{
	}


	CIndividual& CIndividual::operator=(const CIndividual& in)
	{

		if (&in != this)
		{
			//input member
			m_pHost = in.m_pHost;
			m_creationDate = in.m_creationDate;
			m_sex = in.m_sex;
			m_age = in.m_age;
			m_bFertil = in.m_bFertil;
			m_generation = in.m_generation;
			m_scaleFactor = in.m_scaleFactor;

			//state member
			m_status = in.m_status;
			m_death = in.m_death;
			m_broods = in.m_broods;
			m_totalBroods = in.m_totalBroods;
			m_lastAge = in.m_lastAge;
			m_lastStatus = in.m_lastStatus;
		}

		return *this;
	}

	void CIndividual::NewDay()
	{
		m_lastAge = m_age;
		m_lastStatus = m_status;
		m_broods = 0;
	}

	void CIndividual::Live(const CHourlyData& weather, size_t dt)
	{
	}

	void CIndividual::Live(const CWeatherDay& weather)
	{
		assert(IsCreated(weather.GetTRef()) && IsAlive());
	}

	void CIndividual::Brood(const CWeatherDay& weather)
	{
		assert(IsCreated(weather.GetTRef()) && IsAlive());
	}

	void CIndividual::Die(const CWeatherDay& weather)
	{
		assert(IsCreated(weather.GetTRef()) && IsAlive());
	}

	bool CIndividual::CanPack(const CIndividualPtr& in)const
	{
		return GetHost() == in->GetHost() && (abs(GetCreationDate() - in->GetCreationDate()) < 60) && m_broods == 0 && m_totalBroods == 0 && in->m_broods == 0 && in->m_totalBroods == 0;
	}

	void CIndividual::Pack(const CIndividualPtr& in)
	{
		ASSERT(GetStage() == in->GetStage());
		ASSERT(m_status == in->m_status);
		ASSERT(m_death == NOT_DEATH);
		ASSERT(m_generation == in->m_generation);
		ASSERT(m_pHost == in->m_pHost);
		ASSERT(m_scaleFactor > 0 && in->m_scaleFactor > 0);

		m_age = (m_age*m_scaleFactor + in->m_age*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);


		//By RSA: don't mixed total broods because Age is linear and Brood is exponential. By default take only the brood of this insect
		//m_brood= (m_brood*m_scaleFactor + in->m_brood*in->m_scaleFactor)/(m_scaleFactor + in->m_scaleFactor);
		//m_totalBrood = (m_totalBrood*m_scaleFactor + in->m_totalBrood*in->m_scaleFactor)/(m_scaleFactor + in->m_scaleFactor);

		m_scaleFactor += in->m_scaleFactor;

		ASSERT(m_scaleFactor > 0);
	}



	bool CIndividual::CanUnpack()const
	{
		return IsAlive();
	}

	CIndividualPtr CIndividual::Unpack()
	{
		m_scaleFactor /= 2;
		return CreateCopy();
	}

	void CIndividual::HappyNewYear()
	{
	}

	//***********************************************************************************
	CHostPtr CStand::GetNearestHost(CHost* pHost)
	{
		if (pHost == NULL)
			return m_host.empty() ? CHostPtr() : m_host.front();


		ASSERT(false);//"todo"

		return NULL;
	}

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	//
	// Input: double RR: developement (0..1) for this stage
	//
	// Output: TRUE if the insect dies, FALSE auterwise
	//*****************************************************************************
	/*bool CIndividual::IsDeadByAttrition(double RR)const
	{
	bool bDeath=false;

	int s = GetStage();
	if( (HAVE_ATTRITION && SURVIVAL_RATE[s] < 1) )
	{
	//Computes attrition (probability of survival in a given time step, based on development rate)
	double probabSurvival = pow(SURVIVAL_RATE[s], RR);
	if(Randu()>probabSurvival)
	bDeath=true;
	}

	return bDeath;
	}
	*/

	//***************************************************************************
	CHost& CHost::operator = (const CHost& in)
	{
		if (&in != this)
		{
			clear();

			for (const_iterator it = in.begin(); it != in.end(); it++)
			{
				CIndividualPtr pSpeciemen = (*it)->CreateCopy();
				push_front(pSpeciemen);
			}
		}

		return *this;
	}

	double CHost::GetAI(bool bIncludeLast, size_t generation, size_t sex )const
	{
		double AI = CBioSIMModelBase::VMISS;

		if (!empty())
		{
			CStatistic sum;
			CStatistic weight;
			size_t nbStages = (*begin())->GetNbStages();
			for (const_iterator it = begin(); it != end(); it++)
			{
				if ((*it)->GetGeneration() == generation)
				{
					if (sex == NOT_INIT || (*it)->GetSex() == sex)
					{
						double instar = (*it)->GetInstar(bIncludeLast);
						if (instar != CBioSIMModelBase::VMISS)
						{
							sum += instar*(*it)->GetScaleFactor();
							weight += (*it)->GetScaleFactor();
						}
					}
				}
			}

			AI = (weight.IsInit() && weight[SUM] > 0) ? sum[SUM] / weight[SUM] : CBioSIMModelBase::VMISS;
		}

		return AI;
	}

	void CHost::Live(const CWeatherDay& weather)
	{
		//if (m_initialSize == NOT_INIT)
		//m_initialSize = size();

		for (iterator it = begin(); it != end(); it++)
		{
			if ((*it)->IsCreated(weather.GetTRef()))
			{
				(*it)->NewDay();
				if ((*it)->IsAlive())
				{
					(*it)->Live(weather);

					if ((*it)->GetSex() == FEMALE)
						(*it)->Brood(weather);

					(*it)->Die(weather);
				}
			}
		}
	}

	void CHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		//compute stats
		for (iterator it = begin(); it != end(); it++)
		{
			if (generation == NOT_INIT || (*it)->GetGeneration() == generation)
				(*it)->GetStat(d, stat);
		}
	}


	void CHost::HappyNewYear()
	{
		//clean up dead
		for (iterator it = begin(); it != end();)
		{
			(*it)->HappyNewYear();
			if ((*it)->IsAlive())
				it++;
			else
				it = erase(it);
		}
	}

	void CHost::AdjustPopulation()
	{
		size_t nbObjectsAlive = GetNbObjectAlive();
		double nbBugsAlive = GetNbSpecimenAlive();
		if (nbObjectsAlive > 0)
		{
			if (nbObjectsAlive < m_nbMinObjects && (nbBugsAlive / nbObjectsAlive) >= 2)
			{
				UnpackPopulation();
				m_nbPacked--;
			}
			else if (nbObjectsAlive > m_nbMaxObjects)
			{
				PackPopulation();
				m_nbPacked++;
			}
		}
		
	}


	void CHost::PackPopulation()
	{
		if (!empty())
		{
			size_t nbStage = front()->GetNbStages();
			std::vector<iterator> stage;
			stage.insert(stage.begin(), nbStage, end());

			std::vector<std::vector<iterator>> lastAlive;
			lastAlive.insert(lastAlive.begin(), GetNbGeneration(), stage);

			for (iterator it = begin(); it != end();)
			{
				bool bErase = false;
				if ((*it)->IsAlive())
				{
					size_t g = (*it)->GetGeneration();
					size_t s = (*it)->GetStage();
					if (lastAlive[g][s] == end())
					{
						lastAlive[g][s] = it;
					}
					else
					{
						ASSERT((*it)->GetGeneration() == (*lastAlive[g][s])->GetGeneration());
						ASSERT((*it)->GetStage() == (*lastAlive[g][s])->GetStage());

						if ((*lastAlive[g][s])->CanPack(*it))
						{
							(*lastAlive[g][s])->Pack(*it);
							lastAlive[g][s] = end();
							bErase = true;
						}
						else
						{
							//let the old one throws away and try to find with this one
							lastAlive[g][s] = it;
						}
					}
				}//is alive

				if (bErase)
					it = erase(it);
				else
					it++;
			}//all bugs
		}
	}

	void CHost::UnpackPopulation()
	{
		for (const_iterator it = begin(); it != end(); it++)
		{
			if ((*it)->CanUnpack())
			{
				_ASSERTE((*it)->GetScaleFactor() > 0);
				push_front((*it)->Unpack());
			}
		}
	}

	//***************************************************************************




	void CStand::FixAI(double fixeAI)
	{
		//virtual void GetStat(CTRef d, & stat, size_t generation = NOT_INIT)	{ GetFirstHost()->GetStat(d, stat, generation); }
		for (size_t i = 0; i < 10; i++)
		{
			//CModelStat statTmp;
			double AI = GetAI(false);//nerver include the last stage in AI
			//a fix is supply: adjust mean age
			//double simAI = statTmp[CSBStat::AVERAGE_INSTAR];
			double delta = fixeAI - AI;
			if (delta == 0)
				break;

			GetFirstHost()->FixAI(delta);
		}

	}

	void CStand::Live(const CWeatherDay& weather){ ASSERT(GetFirstHost() != NULL); GetFirstHost()->Live(weather); }
	void CStand::GetStat(CTRef d, CModelStat& stat, size_t generation)	{ GetFirstHost()->GetStat(d, stat, generation); }
	void CStand::AdjustPopulation()	{ GetFirstHost()->AdjustPopulation(); }
	size_t CStand::GetNbObjectAlive()const{ return GetFirstHost()->GetNbObjectAlive(); }
	void CStand::HappyNewYear(){ GetFirstHost()->HappyNewYear(); }
	double CStand::GetAI(bool bIncludeLast)const{ return GetFirstHost()->GetAI(bIncludeLast); }


	//***************************************************************************
	//CAgeFrequency

	//CAgeFrequency::CAgeFrequency(CTRef TRef, size_t nbBugs, double age)
	//{
	//	m_month = month;
	//	m_day = day;
	//	m_nbBugs=nbBugs;
	//	m_age=age;
	//}

	size_t CAgeFrequencyVector::GetNbBugs()const
	{
		size_t nbBugs = 0;
		for (size_t i = 0; i < size(); i++)
			nbBugs += at(i).m_scaleFactor;

		return nbBugs;
	}

	void CAgeFrequencyVector::LoadDefault(CTRef TRef, size_t nbBugs, double age)
	{
		clear();
		Initialize(TRef, 0, 400, nbBugs, age);
	}

	ERMsg CAgeFrequencyVector::Load(std::string& filePath)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filePath);
		if (msg)
		{

			string line;
			getline(file, line);//read header
			while (getline(file, line) && !line.empty())
			{
				StringVector elem(line, " \t,;");
				assert(elem.size() == 5);

				//int pos =0;
				int year = ToInt(elem[0]);
				int month = ToInt(elem[1]);
				int day = ToInt(elem[2]) - 1;
				size_t nbBugs = ToSizeT(elem[4]);
				size_t age = ToDouble(elem[5]);

				CTRef TRef(year, month - 1, day - 1);
				ASSERT(TRef.IsValid());

				for (int i = 0; i < nbBugs; i++)
					push_back(CIndividualInfo(TRef, age));
			}

			file.close();
		}

		return msg;
	}


}