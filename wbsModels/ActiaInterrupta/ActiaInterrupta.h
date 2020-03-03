//*****************************************************************************
// Class: CActiaInterrupta, CActiaInterruptaHost, CActiaInterruptaStand
//
// Description:	CActiaInterrupta represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CActiaInterruptaHost represent the tree that contain CActiaInterrupta. 
//				CActiaInterruptaStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "ActiaInterruptaEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace ActiaInterrupta
	{
		enum TActiaInterruptaStats
		{
			S_EGG, S_PUPA, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_ATTRITION, S_HOST_DIE, S_CUMUL_REATCH_ADULT, S_DIAPAUSE, 
			M_EGG, M_PUPA, M_ADULT, M_DEAD_ADULT, M_OVIPOSITING_ADULT, M_BROOD, M_ATTRITION, M_FROZEN, M_HOST_DIE, M_OTHERS, M_DIAPAUSE, M_DIAPAUSE_AGE,
			NB_STATS
		};

	}


	typedef std::array<double, ActiaInterrupta::NB_STAGES> ActiaInterruptaArray;

	class CActiaInterruptaHost;
	class CActiaInterruptaStand;
	class CActiaInterrupta : public CIndividual
	{
	public:

		CActiaInterrupta(CHost* pHost, CTRef creationDate = CTRef(), double age = ActiaInterrupta::EGG, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CActiaInterrupta(const CActiaInterrupta& in) :CIndividual(in){ operator=(in); }
		CActiaInterrupta& operator=(const CActiaInterrupta& in);
		~CActiaInterrupta(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual bool CanPack(const CIndividualPtr& in)const;
		virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return ActiaInterrupta::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CActiaInterrupta>(*this); }
		virtual std::string get_property(const std::string& name)override;

	protected:

		bool IsDeadByAttrition(size_t s, double T);

		//member
		ActiaInterruptaArray m_δ;		//Individual's relative development rates
		//ActiaInterruptaArray m_luck;	//survival between stage
		//bool	m_badluck;		//killed by attrition
		double	m_Pmax;			//Potential fecundity
		double	m_Pᵗ;			//Energy
		double	m_Eᵗ;			//Actual number of eggs

		CTRef	m_adultDate;
		CTRef	m_diapauseTRef;
		double	m_Nh;			//Number of hosts (C. rosaceana) that are in larval stages, excluding L3D;

		

		inline CActiaInterruptaHost* GetHost();
		inline const CActiaInterruptaHost* GetHost()const;
		inline CActiaInterruptaStand* GetStand();
		inline const CActiaInterruptaStand* GetStand()const;
		inline CActiaInterruptaEquations& Equations();
	};
	
	
	//*******************************************************************************************************
	//*******************************************************************************************************
	// CActiaInterruptaHost
	class CActiaInterruptaHost : public CHost
	{
	public:
		//public members
		
		//size_t m_hostType;//kind of host

		CActiaInterruptaHost(WBSF::CStand* pStand);

		//virtual void Live(const CWeatherDay& weaDay)override;
		//virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;
		


		//void CleanUp();
		//bool GetHatchDate(CTRef& ref, double& d)const;

		//bool m_bAutumnCleaned;
		//Defoliation
		//	double m_budDensity;//number of bud by branche
		//	double GetDefoliation()const{ return 1-m_foliageRatio; }
		//	void Eated(double feding){ m_bugsFeeding += feding;}
		//	void UpdateDefoliation();

		//	static double GetHU(CDailyWaveVector& T);
		//	int m_initialNumInd;
		//	double m_bugsFeeding;
		//	double m_foliageRatio;
		//	double m_hu;
	};

	typedef std::shared_ptr<CActiaInterruptaHost> CActiaInterruptaHostPtr;

	
	//*******************************************************************************************************
	//*******************************************************************************************************
	// CActiaInterruptaStand
	class CActiaInterruptaStand : public CStand
	{
	public:

		//global variables of all bugs
		/*bool	m_bApplyAttrition;
		double	m_generationAttrition;
		bool	m_bAutoComputeDiapause;*/
		//double	m_diapauseAge;
		//double	m_lethalTemp;
		double	m_criticalDaylength;


		CActiaInterruptaStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			/*m_bApplyAttrition = true;
			m_generationAttrition = 0.10;
			m_bAutoComputeDiapause = true;
			m_diapauseAge = 0.1;
			m_lethalTemp = -5.0;*/
			m_criticalDaylength = 13.5;
		}

		CActiaInterruptaEquations m_equations;
	};

	//typedef std::shared_ptr<CActiaInterruptaStand> CActiaInterruptaStandPtr;

	//WARNING: cast must be defined here to avoid bug
	inline CActiaInterruptaHost* CActiaInterrupta::GetHost(){ return dynamic_cast<CActiaInterruptaHost*>(m_pHost); }
	inline const CActiaInterruptaHost* CActiaInterrupta::GetHost()const{ return dynamic_cast<const CActiaInterruptaHost*>(m_pHost); }
	inline CActiaInterruptaStand* CActiaInterrupta::GetStand(){ ASSERT(m_pHost); return static_cast<CActiaInterruptaStand*>(m_pHost->GetStand()); }
	inline const CActiaInterruptaStand* CActiaInterrupta::GetStand()const{ ASSERT(m_pHost); return static_cast<const CActiaInterruptaStand*>(m_pHost->GetStand()); }
	inline CActiaInterruptaEquations& CActiaInterrupta::Equations(){ return GetStand()->m_equations; }
}