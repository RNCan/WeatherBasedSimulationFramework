#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TFallInstar{ F_EGG, F_L1, F_L2, F_L3, F_L4, F_SOIL, F_PUPA, F_ADULT, NB_FALL_STAGES };
	enum TFallParam{ F_EGG_L1, F_L1_L2, F_L2_L3, F_L3_L4, F_L4_SOIL, F_SOIL_PUPA, NB_FALL_PARAMS };
	enum TSpringInstar { S_EGG, S_L1, S_L2, S_L3, S_L4, S_L5, S_SOIL, S_PUPA, S_ADULT, NB_SPRING_STAGES };
	enum TSpringParam { S_EGG_L1, S_L1_L2, S_L2_L3, S_L3_L4, S_L4_L5, S_L5_SOIL, NB_SPRING_PARAMS };
	enum TParam { NB_PARAMS_MAX = NB_SPRING_PARAMS};

	extern const char FallCankerworms_header[];
	extern const char SpringCankerworms_header[];
	
	typedef CContinuingRatio<NB_FALL_PARAMS, F_EGG, F_PUPA, FallCankerworms_header> CFallCankerwormsCR;
	typedef CContinuingRatio<NB_SPRING_PARAMS, S_EGG, S_SOIL, SpringCankerworms_header> CSpringCankerwormsCR;

	class CCankerwormsModel : public CBioSIMModelBase
	{
	public:

		enum TSpecies { FALL, SPRING, NB_SPECIES};

		CCankerwormsModel();
		virtual ~CCankerwormsModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CCankerwormsModel; }

	protected:

		size_t m_species;
		CFallCankerwormsCR m_fallCR;
		CSpringCankerwormsCR m_springCR;

		
		//Simulated Annealing 
		enum TInput { I_L1, I_L2, I_L3, I_L4, I_L4_SOIL, I_PUPE, I_ADULT, NB_INPUT };
		int m_firstYear;
		int m_lastYear;

		CStatistic m_DDStat;
		std::array<CStatistic, NB_FALL_PARAMS> m_stageStat;

		static const double A[NB_SPECIES][NB_PARAMS_MAX];
		static const double B[NB_SPECIES][NB_PARAMS_MAX];
	};

}