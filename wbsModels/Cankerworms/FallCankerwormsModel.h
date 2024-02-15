#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TFallInstar{ F_EGG, F_L1, F_L2, F_L3, F_L4, F_SOIL, F_PUPA, F_ADULT, NB_FALL_STAGES };
	enum TFallParam{ F_EGG_L1, F_L1_L2, F_L2_L3, F_L3_L4, F_L4_SOIL, F_SOIL_PUPA, NB_FALL_PARAMS };

	extern const char FallCankerworms_header[];
	
	typedef CContinuingRatio<NB_FALL_PARAMS, F_EGG, F_PUPA, FallCankerworms_header> CFallCankerwormsCR;

	class CFallCankerwormsModel : public CBioSIMModelBase
	{
	public:

		CFallCankerwormsModel();
		virtual ~CFallCankerwormsModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual bool GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CFallCankerwormsModel; }

	protected:

		CFallCankerwormsCR m_fallCR;

		
		//Simulated Annealing 
		enum TFallInput { I_F_L1, I_F_L2, I_F_L3, I_F_L4, I_F_L4_SOIL, I_F_PUPE, I_F_ADULT, NB_FALL_INPUT };

		static const double A[NB_FALL_PARAMS];
		static const double B[NB_FALL_PARAMS];
		static const double THRESHOLD;
	};

}