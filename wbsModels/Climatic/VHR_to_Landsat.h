#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{
	
	class CVHR2LandsatModel : public CBioSIMModelBase
	{
	public:


		CVHR2LandsatModel();
		virtual ~CVHR2LandsatModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteAnnual();
		
		static CBioSIMModelBase* CreateObject(){ return new CVHR2LandsatModel; }

	protected:

		//size_t m_nb_years;

		static double CDD(const CWeatherYear& weather, double threshold = 5);
		static double SWI(const CWeatherYear& weather);
		static double MST(const CWeatherYear& weather);
		static double MWT(const CWeatherYears& weather, int year);
		static double MSP(const CWeatherYear& weather);
		static double MWP(const CWeatherYears& weather, int year);
		static double MCMT(const CWeatherYear& weather);
		static double MWMT(const CWeatherYear& weather);

	};
}