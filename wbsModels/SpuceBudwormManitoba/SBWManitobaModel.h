#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TInstar{ EGG, L1, L2, L3, L4, L5, L6, PUPA, ADULT, DEAD_ADULT, NB_STAGES };
	enum TParam{ P_L2_L3, P_L3_L4, P_L4_L5, P_L5_L6, P_L6_PUPA, P_PUPA_ADULT, NB_PARAMS };
	extern const char SBWM_header[];
	
	typedef CContinuingRatio<NB_PARAMS, L2, PUPA, SBWM_header> CSBWContinuingRatio;

	class CSBWManitobaModel : public CBioSIMModelBase
	{
	public:

		enum TModel{ CONTINUING_RATIO, DEGREE_DAY, NB_MODEL };
		enum TSubModel{ SOUTH_EAST, NORTH_WEST, NB_SUB_MODEL };

		CSBWManitobaModel();
		virtual ~CSBWManitobaModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CSBWManitobaModel; }

	protected:

		int m_model;		//Continouing ratio or Degree-Day
		int m_subModel;		//SouthEast or NortWest
		CSBWContinuingRatio m_continuingRatio;

		//Simulated Ennaling 
		enum TInput { I_NB_IND, I_AI, I_L2, I_L3, I_L4, I_L5, I_L6, I_PUPE, NB_INPUT };
		int m_firstYear;
		int m_lastYear;
		//bool m_bInit;
		CStatistic m_DDStat;
		std::array<CStatistic, NB_PARAMS> m_stageStat;

		static const double A[NB_MODEL][NB_SUB_MODEL][NB_PARAMS];
		static const double B[NB_MODEL][NB_SUB_MODEL][NB_PARAMS];
	};

}