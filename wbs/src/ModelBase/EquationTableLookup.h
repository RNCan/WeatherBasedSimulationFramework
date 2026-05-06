//******************************************************************************
//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once 

#include "Basic/UtilMath.h"

//Weather-Based Simulation Framework
namespace WBSF
{

	class CEquationTableLookup
	{
	public:

		enum TType { T_DEVELOPMENT, T_SURVIVAL, T_FECUNDITY, NB_TYPES };

		//constructor : init rates
		CEquationTableLookup(const CRandomGenerator& RG, size_t nbEquation, double Tmin = -40, double Tmax = 40, double deltaT = 0.25) :
			m_randomGenerator(RG)
		{
			ASSERT(m_deltaT < 1);

			m_nbEquation = nbEquation;
			m_Tmin = Tmin;
			m_Tmax = Tmax;
			m_deltaT = deltaT;
			m_nbTemperature = size_t((m_Tmax - m_Tmin) / m_deltaT);
			assert(m_Tmax == m_Tmin + m_nbTemperature * m_deltaT);
		}


		~CEquationTableLookup()
		{
		}


		void Reinit()
		{
			m_lookup_table.clear();
			///m_lookup_table_survival.clear();
		}

		double GetRate(TType type, size_t stage, double T)const
		{
			double r = 0.0;
			switch (type)
			{
			case T_DEVELOPMENT: r = GetDailyDevlopmentRate(stage, T); break;
			case T_SURVIVAL: r = GetStageSurvival(stage, T); break;
			default: assert(false);
			}

			return r;
		}

		double GetDailyDevlopmentRate(size_t stage, double T)const
		{
			Init();
			return m_lookup_table[T_DEVELOPMENT][GetTIndex(T)][stage];
		}

		double GetStageSurvival(size_t stage, double T)const
		{
			Init();
			return m_lookup_table[T_SURVIVAL][GetTIndex(T)][stage];
		}

		double GetOvipositionRatio(double T)const
		{
			Init();
			return m_lookup_table[T_FECUNDITY][GetTIndex(T)][0];
		}

		//Save the table to a file
		ERMsg Save(const char* filePath);


	protected:


		//These method must be override
		virtual double ComputeDailyDevlopmentRate(size_t e, double T)const = 0;
		virtual double ComputeDailySurvivalRate(size_t e, double T)const
		{
			return 1.0;
		}

		virtual double ComputeOvipositionRatio( double T)const
		{
			return 0.0;
		}


		//virtual double ComputeRate(size_t e, double t)const = 0;
		//virtual bool HasChange()const { return false; }



		void Init(bool bForceInit = false)const;

		//Get the index for a temperature
		//if temperature exceed bound then index is limited to bound
		size_t GetTIndex(double T)const
		{
			int index = (int)Round((T - m_Tmin) / m_deltaT);
			index = std::max(0, std::min(index, (int)m_nbTemperature - 1));

			assert(index >= 0 && index < (int)m_nbTemperature);
			return (size_t)index;
		}

		double m_Tmin;
		double m_Tmax;
		double m_deltaT;
		size_t m_nbEquation;
		size_t m_nbTemperature;

		std::vector < std::vector<std::vector<double>>> m_lookup_table;

		const CRandomGenerator& m_randomGenerator;
	};
}