//*****************************************************************************
// File: CBagworm.h
//
// Class: 
//          
//
// Descrition:	
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase\IndividualBase.h"

namespace WBSF
{


	class CModelStat;
	class CWeatherDay;
	class CDailyWaveVector;

	enum TCBagwormStages{ EGG_DIAPAUSE, EGG, LARVAL, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
	enum TCBagwormStat{ STAT_EGG_DIAPAUSE, STAT_EGG, STAT_LARVAL, STAT_ADULT, STAT_DEAD_ADULT, STAT_NB_BROOD, STAT_NB_CLUSTER, STAT_CLUSTER_WEIGHT, STAT_DEAD_FROZEN, STAT_DEAD_OTHERS, NB_BAGWORM_STAT };
	typedef CModelStatVectorTemplate<NB_BAGWORM_STAT> CBagwormOutputVector;

	class CBagworm : public CIndividual
	{
	public:

		CBagworm(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CBagworm(const CBagworm& in) : WBSF::CIndividual(in){ operator=(in); }
		CBagworm& operator=(const CBagworm& in);
		~CBagworm(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual size_t GetNbStages()const { return NB_STAGES; }

		virtual WBSF::CIndividualPtr CreateCopy()const{ return std::make_shared<CBagworm>(*this); }
		/*{
			CBagworm* pBug = new CBagworm(m_creationDate, m_age, m_bFertil, m_generation, m_scaleFactor, m_pHost);
			pBug->m_clusterWeight = m_clusterWeight;
			return  pBug;
		}
*/

		double GetRelativeDevRate(int s)const { _ASSERTE(s >= 0 && s < NB_STAGES); return m_relativeDevRate[s]; } //Reports individual's relative development rate in "stage" 

		//void SetMinimumTemperatureDate(CTRef date);
		void SetClusterWeight(double clusterWeight){ m_clusterWeight = clusterWeight; }

	protected:

		void Develop(CTRef date, double T, size_t nbStep);
		double GetDevRate(size_t s, double T)const;

		//member
		double m_relativeDevRate[NB_STAGES]; //Individual's relative development rates in 2 stages

		double m_clusterWeight;
		//double m_fec;

		//for survival: Tmax of the coldest (Tmin) day
		CTRef m_dayWhenMinimum;
		CStatistic m_Tday;

	};

//	typedef CHostTemplate<CBagworm> CBWHost;
	//typedef CBWHost::CTBugVector CBWBugVector;
}