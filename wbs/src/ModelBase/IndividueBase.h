//Weather-Based Simulation Framework Individue mod
//
#pragma once

#include <vector>
#include <deque>
#include <list>
#include <algorithm>

#include "Basic/UtilStd.h"
#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/WeatherBasedSimulation.h"

//Weather-Based Simulation Framework
namespace WBSF
{
	class CStand;
	typedef std::shared_ptr<CStand> CStandPtr;


	class CHost;
	typedef std::shared_ptr<CHost> CHostPtr;
	typedef std::list<CHostPtr> CHostPtrList;


	class CIndividue;
	typedef std::shared_ptr<CIndividue> CIndividuePtr;

	class CIndividue
	{
	public:


		static const size_t NOT_DEATH = size_t(-1);

		enum TStatus{ HEALTHY, DEAD, NB_STATUS };//MORIBUND, 
		enum TDeath{ OLD_AGE, FROZEN, MISSING_ENERGY, ATTRITION, ASYNCHRONY, INTOXICATED, WINDOW, TREE_DEFENSE, DENSITY_DEPENDENCE, OTHERS, NB_DEATHS };
		enum TFecondity{ UNFERTIL, FERTIL, NB_FECONDITY };


		CIndividue(CHost* pHost, CTRef creation = CTRef(), double age = 0, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CIndividue(const CIndividue& in){ operator=(in); }
		virtual ~CIndividue(void);

		CIndividue& operator=(const CIndividue& in);

		virtual void NewDay();
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat) = 0;

		virtual bool NeedOverheating()const{ return false; }
		virtual void FixAI(double delta){ m_age = std::max(0.0, m_age + delta); }
		virtual CIndividuePtr CreateCopy()const = 0;
		virtual size_t GetNbStages()const = 0;

		virtual bool CanPack(const CIndividuePtr& in)const;
		virtual void Pack(const CIndividuePtr& in);
		virtual bool CanUnpack()const;
		virtual CIndividuePtr Unpack();
		virtual double GetInstar(bool includeLast)const{ return (IsAlive() || m_death == OLD_AGE) ? (std::min(GetStage(), GetNbStages() - (includeLast ? 0 : 1))) : CBioSIMModelBase::VMISS; }				//Report Instar. Return age by default;


		size_t GetStage()const{ return (size_t)m_age; }					//Reports individual's stage
		size_t GetStatus()const { return m_status; }                    //Reports individual's status
		size_t GetDeath()const{ return m_death; }						//Reason of death (NOT_DEATH for HEALTY individue)
		double GetAge()const { return m_age; }                            //Reports individual's age
		double GetStageAge()const{ return m_age - GetStage(); }				//Report age into the current stage [0:1]


		size_t GetLastStatus()const{ return m_lastStatus; }				//Reports individual's stage at day¯¹
		size_t GetLastStage()const{ return(size_t)m_lastAge; }
		double GetLastAge()const{ return m_lastAge; }					//Reports individual's age at day¯¹

		bool IsChangingStage()const{ return GetStage() != GetLastStage(); }
		bool IsChangingStatus()const{ return m_status != m_lastStatus; }
		bool IsAlive()const{ return m_status == HEALTHY; }				//Reports individual not dead (1) or dead (0) 
		bool IsCreated(CTRef ref)const{ return ref >= m_creationDate; }	//

		size_t GetGeneration()const{ return m_generation; }
		void SetGeneration(size_t in){ m_generation = in; }
		size_t GetSex()const{ return m_sex; }
		void SetSex(size_t in){ m_sex = in; }

		CTRef GetCreationDate()const{ return m_creationDate; }

		double GetScaleFactor()const{ ASSERT(m_scaleFactor > 0); return m_scaleFactor; }
		void SetScaleFactor(double scaleFactor){ ASSERT(scaleFactor > 0); m_scaleFactor = scaleFactor; }

		CHost* GetHost(){ return m_pHost; }
		const CHost* GetHost()const{ return m_pHost; }
		void SetHost(CHost* pHost){ m_pHost = pHost; }

		double  GetBroods()const { return m_broods; }
		double  GetTotalBroods()const { return m_totalBroods; }


		inline CBioSIMModelBase* GetModel();
		inline const CBioSIMModelBase* GetModel()const;
		inline CStand* GetStand();

		const CTimeStep& GetTimeStep()const{ ASSERT(GetModel());	return GetModel()->GetTimeStep(); }
		const CRandomGenerator& RandomGenerator()const{ ASSERT(GetModel());	return GetModel()->RandomGenerator(); }

	protected:

		bool IsChangingStage(double RR)const{ return short(m_age + RR) != GetStage(); }

		//input member
		CHost* m_pHost;			//host on witch insect live
		CTRef m_creationDate;	//creation date
		size_t m_sex;			//sex (male or female)
		double m_age;			//physiological age (0: just hatched. 1: bettle dead of old age)
		bool m_bFertil;			//if female is fertil, they will create new feneration
		size_t m_generation;	//generation of this individu (from 0 to ...)
		double m_scaleFactor;	//How many individues this object represent


		//state member
		size_t m_status;		//State of the individual (alive, dead, inhibited, intoxicated...)
		size_t m_death;			//Cause of death
		double m_broods;		//number of eggs for this day
		double m_totalBroods;	//Cumulative number of eggs for this object
		double m_lastAge;		//physiological age at day-1
		size_t m_lastStatus;	//State of the individual at day-1

	};



	typedef std::list<CIndividuePtr> CIndividuePtrContainer;
	class CHost : public CIndividuePtrContainer
	{
	public:

		size_t m_nbMinObjects;
		size_t m_nbMaxObjects;

		CHost(CStand* pStand) :
			m_pStand(pStand)
		{
			m_initialPopulation = 0;
			m_nbPacked = 0;
			m_nbMinObjects = 100;
			m_nbMaxObjects = 1000;

		}

		CHost(const CHost& in) :
			m_pStand(in.m_pStand)
		{
			operator=(in);
		}

		~CHost()
		{
			clear();
		}

		CHost& operator=(const CHost& in);

		void clear()
		{
			m_initialPopulation = 0;
			m_nbPacked = 0;
		}

		//Generate initial population from CProportionStatVector
		template <class T>
		void Initialize(const CInitialPopulation& initValue)
		{
			clear();

			for (CInitialPopulation::const_iterator it = initValue.begin(); it != initValue.end(); it++)
			{
				push_back(std::make_shared<T>(this, it->m_creationDate, it->m_age, it->m_sex, it->m_bFertil, it->m_generation, it->m_scaleFactor));
				m_initialPopulation += it->m_scaleFactor;
			}
		}


		void FixAI(double delta)
		{
			for (iterator it = begin(); it != end(); it++)
				if ((*it)->IsAlive())//est-ce correcte???
					(*it)->FixAI(delta);
		}

		double GetMeanAge(size_t generation = NOT_INIT)const
		{
			CStatistic age;

			for (const_iterator it = begin(); it != end(); it++)
				if ((*it)->IsAlive())
					if (generation==NOT_INIT||(*it)->GetGeneration() == generation)
						age += (*it)->GetAge();

			return age[MEAN];
		}

		double GetAI(bool bIncludeLast, size_t generation = 0)const;

		size_t GetNbObjectAlive()const
		{
			size_t nbObjectAlive = 0;

			for (const_iterator it = begin(); it != end(); it++)
			{
				if ((*it)->IsAlive())
					nbObjectAlive++;
			}

			return nbObjectAlive;
		}

		double GetNbSpecimenAlive()const
		{
			double nbBugsAlive = 0;

			for (const_iterator it = begin(); it != end(); it++)
			{
				if ((*it)->IsAlive())
					nbBugsAlive += (*it)->GetScaleFactor();
			}

			return nbBugsAlive;
		}




		virtual void Live(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);
		virtual void HappyNewYear();
		virtual void PackPopulation();
		virtual void UnpackPopulation();

		size_t GetNbGeneration()const
		{
			size_t nbGen = 0;
			for (const_iterator it = begin(); it != end(); it++)
				nbGen = std::max(nbGen, (*it)->GetGeneration() + 1);

			return nbGen;
		}

		double GetGenerationRatio(size_t generation)const
		{
			double currentPopulation = 0;

			for (const_iterator it = begin(); it != end(); it++)
			{
				if ((*it)->IsAlive())
					if ((*it)->GetGeneration() == generation)
						currentPopulation += (*it)->GetScaleFactor();
			}

			return currentPopulation / m_initialPopulation;
		}

		void AdjustPopulation();
		size_t GetNbPacked()const{ return m_nbPacked; }

		CStand* GetStand(){ return m_pStand; }
		const CStand* GetStand()const { return m_pStand; }
		void SetStand(CStand* pStand){ m_pStand = pStand; }

		inline CHostPtr GetNearestHost();
		inline CBioSIMModelBase* GetModel();
		inline const CBioSIMModelBase* GetModel()const;

		const CTimeStep& GetTimeStep()const{ ASSERT(GetModel());	return GetModel()->GetTimeStep(); }
		const CRandomGenerator& RandomGenerator()const{ ASSERT(GetModel());	return GetModel()->RandomGenerator(); }

	protected:

		CStand* m_pStand;
		double m_initialPopulation;
		size_t m_nbPacked;
	};





	class CStand
	{
	public:

		//public member :
		CHostPtrList m_host;

		CStand(CBioSIMModelBase* pModel)
		{
			m_pModel = pModel;
		}

		virtual void Live(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);
		virtual void AdjustPopulation();
		virtual size_t GetNbObjectAlive()const;
		virtual void HappyNewYear();
		virtual double GetAI(bool bIncludeLast)const;

		virtual CHostPtr GetNearestHost(CHost* pHost);
		virtual CHostPtr GetFirstHost(){ return GetNearestHost(NULL); }
		virtual const CHostPtr GetFirstHost()const{ return const_cast<CStand*>(this)->GetNearestHost(NULL); }

		CBioSIMModelBase* GetModel(){ ASSERT(m_pModel); return m_pModel; }
		const CBioSIMModelBase* GetModel()const { ASSERT(m_pModel); return m_pModel; }
		const CTimeStep& GetTimeStep()const{ ASSERT(m_pModel);	return m_pModel->GetTimeStep(); }
		const CRandomGenerator& RandomGenerator()const{ ASSERT(m_pModel);	return m_pModel->RandomGenerator(); }

		void FixAI(double fixAI);

	protected:

		CBioSIMModelBase* m_pModel;
	};



	inline CBioSIMModelBase* CIndividue::GetModel(){ ASSERT(m_pHost); return m_pHost->GetModel(); }
	inline const CBioSIMModelBase* CIndividue::GetModel()const{ ASSERT(m_pHost); return m_pHost->GetModel(); }
	inline CStand* CIndividue::GetStand(){ ASSERT(m_pHost); return m_pHost->GetStand(); }

	inline CHostPtr CHost::GetNearestHost(){ ASSERT(m_pStand); return m_pStand->GetNearestHost(this); }
	inline CBioSIMModelBase* CHost::GetModel(){ ASSERT(m_pStand); return m_pStand->GetModel(); }
	inline const CBioSIMModelBase* CHost::GetModel()const { ASSERT(m_pStand); return m_pStand->GetModel(); }
	//inline const CTimeStep& CHost::GetTimeStep()const{ ASSERT(GetModel());	return GetModel()->GetTimeStep(); }
	//inline const CRandomGenerator& CHost::RandomGenerator()const{ ASSERT(GetModel());	return GetModel()->RandomGenerator(); }


	//***************************************************************************
	//CAgeFrequency
	//class CAgeFrequency
	//{
	//public:
	//	CAgeFrequency(size_t month = 0, size_t day = 0, size_t nbBugs = 0, double age = 0);
	//	
	//	size_t m_month;
	//	size_t m_day;
	//	size_t m_nbBugs;
	//	double m_age;
	//};

	class CAgeFrequencyVector : public CInitialPopulation
	{
	public:

		//int size()const{ return (int) std::vector<CAgeFrequency>::size();}
		void LoadDefault(CTRef TRef, size_t nbBugs, double age);
		ERMsg Load(std::string& filePath);
		size_t GetNbBugs()const;
	};

}