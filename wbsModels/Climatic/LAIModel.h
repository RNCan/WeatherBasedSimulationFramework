#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{


	class CLAIModel : public CBioSIMModelBase
	{
	public:

		//enum TGenType{G_RANDOM, G_MEDIAN, NB_GEN_TYPE };

		CLAIModel();
		virtual ~CLAIModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		

		static CBioSIMModelBase* CreateObject(){ return new CLAIModel; }

		static CStatistic GetLAI(const CWeatherYear& weather, size_t forestCoverType, double quantile);
		static CStatistic GetLAI(const CWeatherMonth& weather, size_t forestCoverType, double quantile);
				

		size_t m_forestCoverType;
		double m_quantile;
		
	};
}