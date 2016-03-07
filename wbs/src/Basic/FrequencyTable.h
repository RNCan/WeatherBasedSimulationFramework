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

#include <vector>
#include <crtdbg.h>


namespace WBSF
{

	class CFrequency
	{
	public:

		CFrequency(double low = 0, double high = 0, double delta = 0);
		virtual ~CFrequency(){};

		void Reset(double low = 0, double high = 0, double delta = 0);
		const CFrequency& operator += (double);
		double operator ()(int i, bool bPourcent = false)const{ return m_nbValues > 0 ? ((bPourcent) ? (double)m_bins[i] / m_nbValues : (double)m_bins[i]) : 0; }
		double operator [](int i)const{ return (double)m_bins[i]; }
		int GetSize()const{ return (int)m_bins.size(); }
		double GetClassValue(int i)const{ _ASSERTE(i >= 0 && i < (int)m_bins.size());  return m_low + (i + 0.5)*m_delta; }

		void RegroupClass(int lesserThan);
		int PeakIndex()const;
	private:

		double m_low;
		double m_high;
		double m_delta;

		int m_nbValues;
		std::vector<int> m_bins;
	};

}