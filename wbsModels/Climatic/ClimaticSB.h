#pragma once


#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CClimaticSB : public CBioSIMModelBase
	{
	public:
		CClimaticSB();
		virtual ~CClimaticSB();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CClimaticSB; }

	protected:

		double m_threshold;
		//long GetConsecutiveDayWithoutFrost(const CWeatherYear& weather, double th = 0);
		double GetUtilDeficitPressionVapeur(const CWeatherYear& weather);
	};


}