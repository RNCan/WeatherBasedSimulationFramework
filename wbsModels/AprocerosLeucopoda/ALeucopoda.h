//*****************************************************************************
// Class: CAprocerosLeucopoda, CTZZHost, CTZZStand
//
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "Basic\DegreeDays.h"
#include "ModelBase/IndividualBase.h"
#include "ALeucopodaEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace TZZ
	{
		enum TAprocerosLeucopodaStats
		{
			S_EGG, S_LARVA, S_PREPUPA, S_PUPA, S_ADULT, S_DEAD_ADULT, S_DEAD_ATTRITION, S_BROOD, S_DIAPAUSE,
			S_M_EGG, S_M_LARVA, S_M_PREPUPA, S_M_PUPA, S_M_ADULT, S_M_DEAD_ADULT,
			//S_DEAD_ATTRITION, S_ADULT_EMERGENCE_CDD, 
			NB_STATS
		};
	}


	//*******************************************************************************************************
	// CAprocerosLeucopoda

	class CTZZHost;
	class CTZZStand;
	class CAprocerosLeucopoda : public CIndividual
	{
	public:


		CAprocerosLeucopoda(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = TZZ::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CAprocerosLeucopoda(const CAprocerosLeucopoda& in) :WBSF::CIndividual(in){ operator=(in); }
		CAprocerosLeucopoda& operator=(const CAprocerosLeucopoda& in);
		~CAprocerosLeucopoda(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return TZZ::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CAprocerosLeucopoda>(*this); }

		inline CTZZHost* GetHost();
		inline const CTZZHost* GetHost()const;
		inline CTZZStand* GetStand();
		inline const CTZZStand* GetStand()const;
		inline const CAprocerosLeucopodaEquations& Equations()const;

		//inline CTRef GetAdultEmergenceBegin(size_t y = 1)const;
		//CTRef GetParentAdultEmergence()const;
		CTRef GetCreationDate(int year)const;
		CTRef GetAdultEmergence(int year)const;
		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		double m_RDR[TZZ::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_dropToGroundDate;
		CTRef m_adult_emergence;
		CTRef m_reachDate[TZZ::NB_STAGES+1];
		bool m_bDiapause;

		//double m_adult_longevity; //adult longevity [days]
		double m_Fi; //individual fecundity
		bool m_bDeadByAttrition;
	};

	//*******************************************************************************************************
	// CTZZHost

	class CTZZHost : public CHost
	{
	public:

		//public members
		CTZZHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CTZZHost> CTZZHostPtr;


	//*******************************************************************************************************
	// CTZZStand
	class CTZZStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CTZZStand(WBSF::CBioSIMModelBase* pModel):
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator())
			//m_DD(CDegreeDays::MODIFIED_ALLEN_WAVE, Th1, Th2)
//			m_DD4(CDegreeDays::MODIFIED_ALLEN_WAVE, 4.0)
		{
			m_bApplyAttrition = false;
			//m_egg_creation_CDD = 0;
			m_diapause_end_NCDD = 0;
			m_adult_emergence_CDD = 0;
			m_generationSurvival = 1;
		}

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		
		CTRef ComputeDiapauseEnd(const CWeatherYear& weather)const;
		
		//CModelStatVector m_Tavg30;
		CAprocerosLeucopodaEquations m_equations;
		//CDegreeDays m_DD;
		//CDegreeDays m_DD4;

		//double m_egg_creation_CDD;
		double m_diapause_end_NCDD;
		double m_adult_emergence_CDD;
		double m_generationSurvival;
		//std::array<CTRef, 2>  m_adultEmergenceBegin;
		//CTRef m_diapause_end; // or adult emergence begin;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CTZZHost* CAprocerosLeucopoda::GetHost(){ return static_cast<CTZZHost*>(m_pHost); }
	inline const CTZZHost* CAprocerosLeucopoda::GetHost()const{ return static_cast<const CTZZHost*>(m_pHost); }
	inline CTZZStand* CAprocerosLeucopoda::GetStand(){ ASSERT(m_pHost); return static_cast<CTZZStand*>(GetHost()->GetStand()); }
	inline const CTZZStand* CAprocerosLeucopoda::GetStand()const{ ASSERT(m_pHost); return static_cast<const CTZZStand*>(GetHost()->GetStand()); }
	inline const CAprocerosLeucopodaEquations& CAprocerosLeucopoda::Equations()const{ return GetStand()->m_equations; }


}