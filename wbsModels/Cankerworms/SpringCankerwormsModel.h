#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TSpringInstar { S_EGG, S_L1, S_L2, S_L3, S_L4, S_L5, S_SOIL, S_PUPA, S_ADULT, NB_SPRING_STAGES };
	enum TSpringParam1 { S_PUPA_ADULT, S_ADULT_EGG, NB_SPRING_PARAMS1};
	enum TSpringParam2 { S_EGG_L1, S_L1_L2, S_L2_L3, S_L3_L4, S_L4_L5, S_L5_SOIL, S_SOIL_PUPA, NB_SPRING_PARAMS2 };

	extern const char SpringCankerworms_header[];
	
	typedef CContinuingRatio<NB_SPRING_PARAMS1, S_EGG-2, 0, SpringCankerworms_header> CSpringCankerwormsCR1;
	typedef CContinuingRatio<NB_SPRING_PARAMS2, S_EGG, S_PUPA, SpringCankerworms_header> CSpringCankerwormsCR2;

	class CSpringCankerwormsModel : public CBioSIMModelBase
	{
	public:

		CSpringCankerwormsModel();
		virtual ~CSpringCankerwormsModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CSpringCankerwormsModel; }

	protected:

		CSpringCankerwormsCR1 m_springCR1;
		CSpringCankerwormsCR2 m_springCR2;

		
		//Simulated Annealing 
		enum TSpringInput { I_S_ADULT, I_S_EGG, I_S_L1, I_S_L2, I_S_L3, I_S_L4, I_S_L5, I_S_L5_SOIL, I_S_PUPE, NB_SPRING_INPUT };

		static const double A1[NB_SPRING_PARAMS1];
		static const double A2[NB_SPRING_PARAMS2];
		static const double B1[NB_SPRING_PARAMS1];
		static const double B2[NB_SPRING_PARAMS2];
		static const double THRESHOLD1;
		static const double THRESHOLD2;
	};

}