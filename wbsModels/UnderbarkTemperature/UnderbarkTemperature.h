#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	//Modeling cold tolerance in the mountain pine beetle, Dendroctonus ponderosae
	//Jacques Régnière 2007
	class CMountainPineUnderbarkT
	{
	public:

		CMountainPineUnderbarkT(const CWeatherDay& weather);

		double GetTbark(size_t h, size_t hourTmax = 16)const;
		//double GetTair()const { return (m_Tmin + m_Tmax) / 2.0; }

	protected:

		double m_Tmin;
		double m_Tmax;
	};

	//*******************************************************************************************
	//model from:
	//Temperatures Experienced by Emerald Ash Borer and Other Wood-boring Beetles in the Under-bark Micro-climate
	//Cold temperature and emerald ash borer: Modeling the minimum under-bark temperature of ash trees in Canada
	//Vermunt 2011

	class CNewtonianAshUnderbarkT
	{
	public:

		CNewtonianAshUnderbarkT(double Tair, double K = 0.095);
		double GetTbark()const;
		double next_step(double Tair);

	protected:

		double m_K;
		double m_Tt;
	};

}