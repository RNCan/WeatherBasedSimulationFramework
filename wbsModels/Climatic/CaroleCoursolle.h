#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CCCModel : public CBioSIMModelBase
	{
	public:


		CCCModel();
		virtual ~CCCModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteAnnual();
		
		static CBioSIMModelBase* CreateObject(){ return new CCCModel; }

	protected:

		size_t m_nb_years;

		double MAT(const CWeatherYear& weather);
		double MCMT(const CWeatherYear& weather);
		double EMT(const CWeatherYear& weather);
		double MWMT(const CWeatherYear& weather);
		double EFFP(const CWeatherYear& weather);
		double NFFP(const CWeatherYear& weather);
		double CHDD(const CWeatherYear& weather, double threshold=0);


		/*double MAT(const CWeatherYears& weather, const CTPeriod& p);
		double MCMT(const CWeatherYears& weather, const CTPeriod& p);
		double EMT(const CWeatherYears& weather, const CTPeriod& p);
		double MWMT(const CWeatherYears& weather, const CTPeriod& p);
		double EFFP(const CWeatherYears& weather, const CTPeriod& p);
		double NFFP(const CWeatherYears& weather, const CTPeriod& p);
		double CHDD(const CWeatherYears& weather, double threshold, const CTPeriod& p);*/


	};
}