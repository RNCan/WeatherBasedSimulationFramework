//Weather-Based Simulation Framework 
//Individual insect base
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


	class CIndividual;
	typedef std::shared_ptr<CIndividual> CIndividualPtr;

	class CIndividual
	{
	public:


		static const size_t NOT_DEATH = size_t(-1);

		enum TStatus{ HEALTHY, DEAD, NB_STATUS };//MORIBUND, 
		enum TDeath{ OLD_AGE, FROZEN, MISSING_ENERGY, ATTRITION, ASYNCHRONY, INTOXICATED, WINDOW, TREE_DEFENSE, DENSITY_DEPENDENCE, EXODUS, HOST_DIE, OTHERS, NB_DEATHS };
		enum TFecundity{ UNFERTIL, FERTIL, NB_FECONDITY };

		
		
		CIndividual(CHost* pHost, CTRef creation = CTRef(), double age = 0, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0, double scaleFactor = 1);
		CIndividual(const CIndividual& in){ operator=(in); }
		virtual ~CIndividual(void);

		CIndividual& operator=(const CIndividual& in);

		virtual void OnNewDay(const CWeatherDay& weather);//let object to be created if not created yet and reset var
		virtual void Live(const CHourlyData& weather, size_t dt);
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat) = 0;
		virtual void HappyNewYear();

		virtual bool NeedOverheating()const{ return false; }
		virtual void FixAI(double delta){ m_age = std::max(0.0, m_age + delta); }
		virtual CIndividualPtr CreateCopy()const = 0;
		virtual size_t GetNbStages()const = 0;

		virtual bool CanPack(const CIndividualPtr& in)const;
		virtual void Pack(const CIndividualPtr& in);
		virtual bool CanUnpack()const;
		virtual CIndividualPtr Unpack();
		virtual double GetInstar(bool includeLast)const{ return (IsAlive() || m_death == OLD_AGE) ? (std::min(GetStage(), GetNbStages() - (includeLast ? 0 : 1))) : CBioSIMModelBase::VMISS; }				//Report Instar. Return age by default;
		virtual bool IsInDiapause(CTRef TRef)const{ return false; }
		//virtual bool IsInDiapause2(CTRef TRef)const{ return false; }
		virtual std::string get_property(const std::string& name)const;


		size_t GetStage()const{ return (size_t)m_age; }					//Reports individual's stage
		size_t GetStatus()const { return m_status; }                    //Reports individual's status
		void SetStatus(size_t status){ m_status = status; }                    
		size_t GetDeath()const{ return m_death; }						//Reason of death (NOT_DEATH for HEALTY individual)
		void SetDeath(size_t death){ m_death = death; }
		double GetAge()const { return m_age; }                            //Reports individual's age
		double GetStageAge()const{ return m_age - GetStage(); }				//Report age into the current stage [0:1]


		size_t GetLastStatus()const{ return m_lastStatus; }				//Reports individual's stage at day��
		size_t GetLastStage()const{ return(size_t)m_lastAge; }
		double GetLastAge()const{ return m_lastAge; }					//Reports individual's age at day��

		bool HasChangedStage()const{ return GetStage() != GetLastStage(); }
		bool IsChangingStage(double r)const{ return GetStage() != (size_t)(m_age+r); }
		bool HasChangedStatus()const{ return m_status != m_lastStatus; }
		bool IsAlive()const{ return m_status == HEALTHY; }				//Reports individual not dead (1) or dead (0) 
		bool IsCreated(CTRef ref)const{ return ref >= m_creationDate; }	//

		size_t GetGeneration()const{ return m_generation; }
		void SetGeneration(size_t in){ m_generation = in; }
		size_t GetSex()const{ return m_sex; }
		void SetSex(size_t in){ m_sex = in; }

		CTRef GetCreationDate()const{ return m_creationDate; }

		double GetScaleFactor()const{ return m_scaleFactor; }
		void SetScaleFactor(double scaleFactor){ m_scaleFactor = scaleFactor; }

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

		

		//input member
		CHost* m_pHost;			//host on witch insect live
		CTRef m_creationDate;	//creation date
		size_t m_sex;			//sex (male or female)
		double m_age;			//physiological age (0: just hatched. 1: beetle dead of old age)
		bool m_bFertil;			//if female is fertile, they will create new generation
		size_t m_generation;	//generation of this individual (from 0 to ...)
		double m_scaleFactor;	//How many individuals this object represent


		//state member
		size_t m_status;		//State of the individual (alive, dead, inhibited, intoxicated...)
		size_t m_death;			//Cause of death
		double m_broods;		//number of eggs for this day
		double m_totalBroods;	//Cumulative number of eggs for this object
		double m_lastAge;		//physiological age at day-1
		size_t m_lastStatus;	//State of the individual at day-1

	};



	typedef std::list<CIndividualPtr> CIndividualPtrContainer;
	class CHost : public CIndividualPtrContainer
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
				if ((*it)->IsAlive())
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

		double GetAI(bool bIncludeLast, size_t generation = 0, size_t sex = NOT_INIT)const;

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
		virtual bool AdjustPopulation();
		virtual std::string get_property(const std::string& name)const;



		size_t GetNbPacked()const{ return m_nbPacked; }

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

		double GetInitialPopulation()const{ return m_initialPopulation; }

		

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
		virtual bool AdjustPopulation();
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



	inline CBioSIMModelBase* CIndividual::GetModel(){ ASSERT(m_pHost); return m_pHost->GetModel(); }
	inline const CBioSIMModelBase* CIndividual::GetModel()const{ ASSERT(m_pHost); return m_pHost->GetModel(); }
	inline CStand* CIndividual::GetStand(){ ASSERT(m_pHost); return m_pHost->GetStand(); }

	inline CHostPtr CHost::GetNearestHost(){ ASSERT(m_pStand); return m_pStand->GetNearestHost(this); }
	inline CBioSIMModelBase* CHost::GetModel(){ ASSERT(m_pStand); return m_pStand->GetModel(); }
	inline const CBioSIMModelBase* CHost::GetModel()const { ASSERT(m_pStand); return m_pStand->GetModel(); }
	

	//***************************************************************************
	//CAgeFrequency

	class CAgeFrequencyVector : public CInitialPopulation
	{
	public:

		void LoadDefault(CTRef TRef, size_t nbBugs, double age);
		ERMsg Load(std::string& filePath);
		size_t GetNbBugs()const;
	};

}