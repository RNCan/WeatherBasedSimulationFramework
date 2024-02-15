#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TSpringInstar { S_EGG, S_L1, S_L2, S_L3, S_L4, S_L5, S_SOIL, S_PUPA, S_ADULT, NB_SPRING_STAGES };
	enum TSpringParam { S_PUPA_ADULT, S_ADULT_EGG, S_EGG_L1, S_L1_L2, S_L2_L3, S_L3_L4, S_L4_L5, S_L5_SOIL, S_SOIL_PUPA, NB_SPRING_PARAMS };

	extern const char SpringCankerworms_header[];
	typedef CContinuingRatio<NB_SPRING_PARAMS, S_EGG-2, S_PUPA, SpringCankerworms_header> CSpringCankerwormsCR;

	class CSpringCankerwormsModel : public CBioSIMModelBase
	{
	public:

		CSpringCankerwormsModel();
		virtual ~CSpringCankerwormsModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual bool GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CSpringCankerwormsModel; }

	protected:

		CSpringCankerwormsCR m_springCR;

		
		//Simulated Annealing 
		enum TSpringInput { I_S_ADULT, I_S_EGG, I_S_L1, I_S_L2, I_S_L3, I_S_L4, I_S_L5, I_S_L5_SOIL, I_S_PUPE, NB_SPRING_INPUT };

		static const double A[NB_SPRING_PARAMS];
		static const double B[NB_SPRING_PARAMS];
		static const double THRESHOLD;

	};

}