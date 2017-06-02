//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
#pragma once

#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"

namespace WBSF
{
	//*******************************************************************************
	//
	class CWaterTemperature
	{
	public:

		static const double α1;
		static const double α2;
		static const double θ1;
		static const double β1;
		static const double β2;
		static const double β3;
		static const double b1;
		static const double b2;
		static const double b3;

		CWaterTemperature(const CWeatherYears& data = CWeatherYears());


		//after Use of Air-Water Relationships for Predicting Water Temperature 
		//by V.Kothandaraman and R.L.Evans
		double GetTwI(const CTRef& TRef) const;
		//after 
		double GetTwII(const CTRef& TRef) const;
		void ComputeRa(const CWeatherYears& data);
		void ComputeRw(const CWeatherYears& data);
		void Compute(const CWeatherYears& data);
		void ComputeParams(const CWeatherYears& data);
		ERMsg Export(const std::string& filePath);

	protected:


		//d: julian day (0..365)
		//Tw: air temperature [°C]
		double GetAnnualCycleTrend(CTRef TRef) const
		{
			double dr = (double)TRef.GetJDay() / TRef.GetNbDaysPerYear();
			assert(dr >= 0 && dr < 1);
			assert((Aᵒ / 2) + (A¹*cos(2 * PI*dr)) + (B¹*sin(2 * PI*dr)) >= -60);
			assert((Aᵒ / 2) + (A¹*cos(2 * PI*dr)) + (B¹*sin(2 * PI*dr)) <= 60);
			return (Aᵒ / 2) + (A¹*cos(2 * PI*dr)) + (B¹*sin(2 * PI*dr));
		}


		double Aᵒ;
		double A¹;
		double B¹;
		double θ;

		CModelStatVector Ra;
		CModelStatVector Rw;

	};

}//namespace WBSF