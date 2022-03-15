#pragma once


#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CClimaticQc : public CBioSIMModelBase
	{
	public:


		enum TGrowingSeason { FIRST_TIME_GREATER, LAST_TIME_SMALLER, NB_GS_TYPE };

		CClimaticQc();
		virtual ~CClimaticQc();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CClimaticQc; }

	protected:

		double m_threshold;
		TGrowingSeason m_growing_season_type;


		
		double GetUtilDeficitPressionVapeur(const CWeatherYear& weather);
		double GetTotalVaporPressureDeficit(const CWeatherYear& weather);

		static double GetVPD(const CWeatherYear& weather);
		static double GetVPD(const CWeatherMonth& weather);
		static double GetVPD(const CWeatherDay& weather);
		

	};


}