#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CClimaticMoistureDeficitModel : public CBioSIMModelBase
	{
	public:


		CClimaticMoistureDeficitModel();
		virtual ~CClimaticMoistureDeficitModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		
		static CBioSIMModelBase* CreateObject(){ return new CClimaticMoistureDeficitModel; }

		
		CModelStatVector GetETo(const CWeatherStation& weather);

	protected:

		size_t m_nb_years;

		static double MAT(const CWeatherYear& weather);
		static double MCMT(const CWeatherYear& weather);
		static double EMT(const CWeatherYear& weather);
		static double MWMT(const CWeatherYear& weather);
		static double NFFD(const CWeatherYear& weather);
		static double CHDD(const CWeatherYear& weather, double threshold=0);
		static double CDD5(const CWeatherYear& weather, double threshold = 5);
		static double PPT5(const CWeatherYear& weather);

	


	};
}