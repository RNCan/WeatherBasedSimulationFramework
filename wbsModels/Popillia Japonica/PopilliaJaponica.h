//*****************************************************************************
// Class: CPopilliaJaponica, CPJNHost, CPJNStand
//
// Description:	CPopilliaJaponica represent a PJN insect or a group of insect with same characteristics. 
//				CPJNHost represent the Host that contain CPopilliaJaponica. 
//				CPJNStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
//#include "ModelBase/ModelDistribution.h"
#include "PopilliaJaponicaEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace PJN
	{


		enum TPopilliaJaponicaStats
		{
			S_TAIR, S_TSOIL, S_SNDH, S_PSMI, S_EGG, S_L1, S_L2, S_L3, S_PUPA, S_ADULT, S_DEAD_ADULT, S_EGG_BROOD, S_EGG_HATCH, S_ADULT_EMERGENCE,
			S_CDD, S_ADULT_CATCH_CDD, S_DEAD_BY_FROST, S_DEAD_ATTRITION, NB_STATS
		};

		
	}


	//*******************************************************************************************************
	// CPopilliaJaponica

	class CPJNHost;
	class CPJNStand;
	class CPopilliaJaponica : public CIndividual
	{
	public:


		CPopilliaJaponica(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = PJN::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CPopilliaJaponica(const CPopilliaJaponica& in) :WBSF::CIndividual(in){ operator=(in); }
		CPopilliaJaponica& operator=(const CPopilliaJaponica& in);
		~CPopilliaJaponica(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		//virtual void Live(const CHourlyData& weather, size_t dt)override;

		void Live(CTRef d, double Tsoil, double Tair, size_t dt);
		
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LOF::L2o ? m_age : std::max(double(LOF::L2o), m_age - 1), double(LOF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return PJN::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CPopilliaJaponica>(*this); }

		inline CPJNHost* GetHost();
		inline const CPJNHost* GetHost()const;
		inline CPJNStand* GetStand();
		inline const CPJNStand* GetStand()const;
		inline const CPopilliaJaponicaEquations& Equations()const;

		bool IsDeadByAttrition()const;

	protected:

		//member
		double m_EOD; //End of diapause CDD
		//CTRef m_EODDate;
		double m_RDR[PJN::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_diapause_date;
		double m_diapause_status;
		double m_diapause_RDR;
		//double m_diapauseCDD;
		double m_r_snow;


		//CTRef m_adult_emergence;
		std::array<CTRef, PJN::NB_STAGES> m_reachDate;
	
		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
		double m_cooling_point;
		
	};

	//*******************************************************************************************************
	// CPJNHost

	class CPJNHost : public CHost
	{
	public:

		//public members
		CPJNHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CPJNHost> CPJNHostPtr;


	//*******************************************************************************************************
	// CPJNStand
	class CPJNStand : public CStand
	{
	public:

		//global variables of all bugs
		CPJNStand(WBSF::CBioSIMModelBase* pModel, const CModelStatVector& soil_temperature, const CModelStatVector& PMSI);
		


		void InitStand(const std::vector<double>& EOD, const std::array<double, NB_CDD_PARAMS >& ACC, std::array<double, PJN::NB_OTHER_PARAMS>& other, const CWeatherYear& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		CPopilliaJaponicaEquations m_equations;
		const CModelStatVector& m_soil_temperature;
		const CModelStatVector& m_PMSI;
		//CTRef m_diapause_end;
		CModelStatVector m_CDD;

		double m_snow;
		double m_TsoilAMJ;
//		double m_sigma;
		
		//bool m_bApplyAttrition;
	//	bool m_bApplyFrost;
		//CModelStatVector m_EOD_CDD;
		//CModelStatVector m_EOD_CDF;
//		double m_overwinter_s;
		//CModelStatVector m_diapause;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CPJNHost* CPopilliaJaponica::GetHost(){ return static_cast<CPJNHost*>(m_pHost); }
	inline const CPJNHost* CPopilliaJaponica::GetHost()const{ return static_cast<const CPJNHost*>(m_pHost); }
	inline CPJNStand* CPopilliaJaponica::GetStand(){ ASSERT(m_pHost); return static_cast<CPJNStand*>(GetHost()->GetStand()); }
	inline const CPJNStand* CPopilliaJaponica::GetStand()const{ ASSERT(m_pHost); return static_cast<const CPJNStand*>(GetHost()->GetStand()); }
	inline const CPopilliaJaponicaEquations& CPopilliaJaponica::Equations()const{ return GetStand()->m_equations; }


}