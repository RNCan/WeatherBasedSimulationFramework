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
			assert(m_Tmax == m_Tmin + m_nbTemperature*m_deltaT);
		}


		~CEquationTableLookup()
		{
		}


		void Reinit()
		{
			m_lookupTable.clear();
		}

		double GetRate(size_t s, double t)const
		{//HaveChange()
			Init();
			return m_lookupTable[GetTIndex(t)][s];
		}

		virtual double ComputeRate(size_t e, double t)const = 0;
		virtual bool HaveChange()const { return false; }

		//Save the table to a file
		ERMsg Save(const char* filePath);


	protected:

		void Init(bool bForceInit = false)const;

		//Get the index for a temperature
		//if temperature exceed bound then index is limited to bound
		size_t GetTIndex(double T)const
		{
			int index = (int)Round((T - m_Tmin) / m_deltaT);
			index = std::max(0, std::min(index, (int)m_lookupTable.size() - 1));

			_ASSERTE(index >= 0 && index < (int)m_lookupTable.size());
			return (size_t)index;
		}

		double m_Tmin;
		double m_Tmax;
		double m_deltaT;
		size_t m_nbEquation;
		size_t m_nbTemperature;

		std::vector<std::vector<double>> m_lookupTable;
		const CRandomGenerator& m_randomGenerator;
	};
}