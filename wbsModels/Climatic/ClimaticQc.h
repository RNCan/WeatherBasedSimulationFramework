#pragma once


#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CClimaticQc : public CBioSIMModelBase
	{
	public:
		CClimaticQc();
		virtual ~CClimaticQc();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CClimaticQc; }

	protected:

		double m_threshold;
		//long GetConsecutiveDayWithoutFrost(const CWeatherYear& weather, double th = 0);
		double GetUtilDeficitPressionVapeur(const CWeatherYear& weather);


		static double GetVPD(const CWeatherYear& weather);
		static double GetVPD(const CWeatherMonth& weather);
		static double GetVPD(const CWeatherDay& weather);

	};


}