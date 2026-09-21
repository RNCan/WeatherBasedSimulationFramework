//*****************************************************************************
// Class: CGypsyMothEggHatch, CLDDHost, CLDDStand
//
// Description:	CGypsyMothEggHatch represent an insect or a group of insect with same characteristics. 
//				CLDDHost represent the Host that contain CGypsyMothEggHatch. 
//				CLDDStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "GypsyMothEggHatchEquations.h"
#include "Basic/DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LDD
	{
		enum TGypsyMothEggHatchStats
		{
			S_DIAPAUSE_EGGS, S_EGGS, S_LARVAE, S_DIAPAUSE_END, S_EGGS_HATCH,
			S_DEAD_ATTRITION, S_DEAD_FROST, S_DIAPAUSE_END_NCDD1, S_DIAPAUSE_END_NCDD2, NB_STATS,
			NB_CUMULATIVE_STATS = 2
		};

		
		static const std::array<size_t, NB_CUMULATIVE_STATS  > CUMULATIVE_STATS = { S_DIAPAUSE_END, S_EGGS_HATCH };

	}


	//*******************************************************************************************************
	// CGypsyMothEggHatch

	class CLDDHost;
	class CLDDStand;
	class CGypsyMothEggHatch : public CIndividual
	{
	public:

		CGypsyMothEggHatch(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LDD::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CGypsyMothEggHatch(const CGypsyMothEggHatch& in) :WBSF::CIndividual(in){ operator=(in); }
		CGypsyMothEggHatch& operator=(const CGypsyMothEggHatch& in);
		~CGypsyMothEggHatch(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LDD::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CGypsyMothEggHatch>(*this); }

		inline CLDDHost* GetHost();
		inline const CLDDHost* GetHost()const;
		inline CLDDStand* GetStand();
		inline const CLDDStand* GetStand()const;
		inline const CGypsyMothEggHatchEquations& Equations()const;

		bool IsDeadByAttrition(size_t stage, double T, double i_r)const;

	protected:

		//member
		std::array<double, LDD::NB_STAGES> m_RDR;	//Individual's relative development rates for all stages
		CTRef m_end_of_diapause;
		CTRef m_deadByAttrition;

		double m_cold_tolerence;
		
	};

	//*******************************************************************************************************
	// CLDDHost

	class CLDDHost : public CHost
	{
	public:

		//public members
		CLDDHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CLDDHost> CLDDHostPtr;


	//*******************************************************************************************************
	// CLDDStand
	class CLDDStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CLDDStand(WBSF::CBioSIMModelBase* pModel) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = false;
			m_EOD_sigma=0;
		}

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

		void init(int year, const CWeatherYears& weather);
		
		
		CGypsyMothEggHatchEquations m_equations;
		CModelStatVector m_diapause_end_NCDD;
		
		CTRef m_EOD_mu;
		double m_EOD_sigma; 
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLDDHost* CGypsyMothEggHatch::GetHost(){ return static_cast<CLDDHost*>(m_pHost); }
	inline const CLDDHost* CGypsyMothEggHatch::GetHost()const{ return static_cast<const CLDDHost*>(m_pHost); }
	inline CLDDStand* CGypsyMothEggHatch::GetStand(){ ASSERT(m_pHost); return static_cast<CLDDStand*>(GetHost()->GetStand()); }
	inline const CLDDStand* CGypsyMothEggHatch::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLDDStand*>(GetHost()->GetStand()); }
	inline const CGypsyMothEggHatchEquations& CGypsyMothEggHatch::Equations()const{ return GetStand()->m_equations; }


}