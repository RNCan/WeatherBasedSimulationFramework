#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	class CMicroClimate
	{
	public:

		CMicroClimate(const CWeatherDay& weather);

		double GetT(size_t h, size_t hourTmax = 16)const;
		double GetTair()const { return (m_Tmin + m_Tmax) / 2.0; }

	protected:

		double m_Tmin;
		double m_Tmax;
	};

	//*******************************************************************************************

	class CNewtonianBarkTemperature
	{
	public:

		CNewtonianBarkTemperature(double Tair, double K = 0.095);
		double GetTbark()const;
		double next_step(double Tair);

	protected:

		double m_K;
		double m_Tt;
	};

}