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

		static double MAT(const CWeatherYear& weather);
		static double MCMT(const CWeatherYear& weather);
		static double EMT(const CWeatherYear& weather);
		static double MWMT(const CWeatherYear& weather);
		static double NFFD(const CWeatherYear& weather);
		static double CHDD(const CWeatherYear& weather, double threshold=0);


		/*double MAT(const CWeatherYears& weather, const CTPeriod& p);
		double MCMT(const CWeatherYears& weather, const CTPeriod& p);
		double EMT(const CWeatherYears& weather, const CTPeriod& p);
		double MWMT(const CWeatherYears& weather, const CTPeriod& p);
		double EFFP(const CWeatherYears& weather, const CTPeriod& p);
		double NFFP(const CWeatherYears& weather, const CTPeriod& p);
		double CHDD(const CWeatherYears& weather, double threshold, const CTPeriod& p);*/


	};
}