//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
//
// Class: CEquationTableLookup
//          
//
// Description: template for optimization of development rates
// template parameters:
// class T: class of the template. The class must implement "double ComputeRate(int e, double t)"
// short NB_EQUATION : The number of equation to compute
// short TMIN : the first temperature in the table
// short TMAX : the last temperature in the table
// short NB_CPD : the number of class per °C
//*****************************************************************************
// 05/05/2026   Rémi Saint-Amant    Add survival in the lookup table
// 25/02/2015   Rémi Saint-Amant    replace CStdStdioFile by ofStream
// 12/10/2012   Rémi Saint-Amant    No longer template
// 24/09/2012   Rémi Saint-Amant    Change in template. remove "static" parameters in development class
// 07/02/2011	Rémi Saint-Amant    Add critical section in initialization
// 14/02/2008   Rémi Saint-Amant    Creation
//*****************************************************************************

#pragma once 

#include "stdafx.h"
#include "Basic/UtilStd.h"
#include "ModelBase/EquationTableLookup.h"


using namespace std;


//Weather-Based Simulation Framework
namespace WBSF
{
	ERMsg CEquationTableLookup::Save(const char* filePath)
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath);

		if (msg)
		{
			file << "T";

			for (size_t s = 0; s < m_nbEquation; s++)
				file << FormatA(",E%d", s + 1);
			for (size_t s = 0; s < m_nbEquation; s++)
				file << FormatA(",S%d", s + 1);
			file << FormatA(",Fo");

			file << endl;

			for (size_t i = 0; i < m_nbTemperature; i++)
			{
				double t = m_Tmin + m_deltaT * i;

				file << FormatA("%.2lf", t);
				for (size_t e = 0; e < m_nbEquation; e++)
					file << FormatA(",%lf", m_lookup_table[T_DEVELOPMENT][i][e]);
				for (size_t e = 0; e < m_nbEquation; e++)
					file << FormatA(",%lf", m_lookup_table[T_SURVIVAL][i][e]);
				file << FormatA(",%lf", m_lookup_table[T_FECUNDITY][i][0]);

				file << endl;
			}

			file.close();
		}

		return msg;
	}


	//template <class T, short NB_EQUATION, short TMIN, short TMAX, short NB_CPD>
	void CEquationTableLookup::Init(bool bForceInit)const
	{
		CEquationTableLookup& me = const_cast<CEquationTableLookup&>(*this);

		//development survival rate are only compute once per model call
		//if (m_lookup_table_development.empty() || 
		//	m_lookup_table_survival.empty() || 
		//	bForceInit)
		//{
		//	//me.m_lookup_table_development.clear();
		//	me.m_lookup_table_development.resize(m_nbTemperature);
		//	//me.m_lookup_table_survival.clear();
		//	me.m_lookup_table_survival.resize(m_nbTemperature);
		//	

		//	for (size_t t = 0; t < m_lookup_table_survival.size(); t++)
		//	{
		//		me.m_lookup_table_development[t].insert(me.m_lookup_table_development[t].begin(), m_nbEquation, 0);
		//		me.m_lookup_table_survival[t].insert(me.m_lookup_table_survival[t].begin(), m_nbEquation, 0);
		//		double T = m_Tmin + m_deltaT * t;
		//		for (size_t e = 0; e < m_lookup_table_survival[t].size(); e++)
		//		{
		//			double r = ComputeDailyDevlopmentRate(e, T);
		//			double s = ComputeDailySurvivalRate(e, T);
		//			me.m_lookup_table_development[t][e] = r;
		//			me.m_lookup_table_survival[t][e] = pow(s, 1.0 / r);
		//			assert(me.m_lookup_table_survival[t][e] >= 0 && me.m_lookup_table_survival[t][e] <= 1);
		//		}
		//	}
		//}

		if (m_lookup_table.empty() || bForceInit)
		{
			me.m_lookup_table.resize(NB_TYPES);
			for (size_t i = 0; i < NB_TYPES; i++)
				me.m_lookup_table[i].resize(m_nbTemperature);

			for (size_t t = 0; t < m_nbTemperature; t++)
			{
				me.m_lookup_table[T_DEVELOPMENT][t].resize( m_nbEquation, 0);
				me.m_lookup_table[T_SURVIVAL][t].resize(m_nbEquation, 1);
				me.m_lookup_table[T_FECUNDITY][t].resize( 1, 0);

				double T = m_Tmin + m_deltaT * t;
				for (size_t e = 0; e < m_nbEquation; e++)
				{

					double r = ComputeDailyDevlopmentRate(e, T);
					double s = ComputeDailySurvivalRate(e, T);
					me.m_lookup_table[T_DEVELOPMENT][t][e] = r;
					me.m_lookup_table[T_SURVIVAL][t][e] = pow(s, 1.0 / r);
					assert(me.m_lookup_table[i][t][e] >= 0 && me.m_lookup_table[i][t][e] <= 1);


				}

				double o = ComputeOvipositionRatio(T);
				me.m_lookup_table[T_FECUNDITY][t][0] = o;

			}
		}

	}


}