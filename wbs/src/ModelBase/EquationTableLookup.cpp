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
// Descrition: template for optimization of development rates
// template parameters:
// class T: class of the template. The class must implement "double ComputeRate(int e, double t)"
// short NB_EQUATION : The number of equation to compute
// short TMIN : the first temperature in the table
// short TMAX : the last temperature in the table
// short NB_CPD : the number of class per °C
//*****************************************************************************
// 25/02/2015   Rémi Saint-Amant    replace CStdStdioFile by ofStream
// 12/10/2012   Rémi Saint-Amant    Untemplate-it
// 24/09/2012   Rémi Saint-Amant    Change in template. remove "static" parameters in developement class
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

			file << endl;

			for (size_t i = 0; i < m_nbTemperature; i++)
			{
				double t = m_Tmin + m_deltaT*i;

				file << FormatA("%.2lf", t);
				for (int s = 0; s < m_nbEquation; s++)
					file << FormatA(",%lf", m_lookupTable[i][s]);

				file << endl;
			}

			file.close();
		}

		return msg;
	}


	//template <class T, short NB_EQUATION, short TMIN, short TMAX, short NB_CPD>
	void CEquationTableLookup::Init(bool bForceInit)const
	{
		//development rate are only compute once
		if (m_lookupTable.empty() || bForceInit)
		{
			CEquationTableLookup& me = const_cast<CEquationTableLookup&>(*this);

			me.m_lookupTable.clear();
			me.m_lookupTable.resize(m_nbTemperature);
			for (size_t i = 0; i < me.m_lookupTable.size(); i++)
				me.m_lookupTable[i].insert(me.m_lookupTable[i].begin(), m_nbEquation, 0);

			for (size_t i = 0; i < m_lookupTable.size(); i++)
			{
				double t = m_Tmin + m_deltaT*i;
				for (size_t s = 0; s < m_lookupTable[i].size(); s++)
				{
					me.m_lookupTable[i][s] = ComputeRate(s, t);
					assert(me.m_lookupTable[i][s] >= 0 /*&& me.m_lookupTable[i][s] <= 1*/);
				}
			}
		}
	}
}