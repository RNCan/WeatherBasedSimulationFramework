//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Jacques Régnière, Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
//
// Abstract:    This class simulate daily air temperature, precipitation, humidity and wind speed from monthly normal
//
// See article : Régnière, J., St-Amant, R. 2007. Stochastic simulation of daily air 
//				 temperature and precipitation from monthly normals in Noth America north 
//				 of Mexico. Int J Biometeorol. 
//
//***************************************************************************
#pragma once

#include <array>
#include "Basic/WeatherDefine.h"
#include "Basic/AmountPpt.h"
#include "Basic/UtilStd.h"
#include "Basic/NormalsStation.h"

namespace WBSF
{
	class CWeatherYear;


	typedef std::array<int, HOURLY_DATA::NB_VAR_H> CSeed;
	typedef std::vector<CSeed> CSeedVector;
	typedef std::vector<CSeedVector> CSeedMatrix;

	class CWeatherGeneratorKernel
	{
	public:

		CWeatherGeneratorKernel(CWVariables m_variables = "TN T TX P");
		virtual ~CWeatherGeneratorKernel();

		void SetNormals(const CNormalsData& normals);
		void SetVaraibles(CWVariables in){ m_variables = in; }
		void ResetDeltaEpsilon();

		void SetSeed(const CSeed& seed){ m_seed = seed; }
		void Generate(CWeatherYear& dailyData);


	private:

		void InitRandomNumber(int m, double p_test[31], double epsilon_prec[31]);
		void RandomizeNormals(CNormalsData& normalTmp);

		void GetRandomTraces(double e_min[31], double e_max[31], size_t month);
		void DailyPrecipitation(int month, const CNormalsData& normalTmp, CWeatherYear& dailyData);

		CNormalsData m_normals;
		CAmountPpt m_AP;

		void InitDeltaEpsilon();
		bool IsDeltaEpsilonInit(){ return (m_delta[0] + m_delta[1] + m_delta[2]) != 0; }

		CWVariables m_variables;

		double m_delta[3];
		double m_epsilon[3];
		CSeed m_seed;
		CRandomGenerator m_rand;
	};

}