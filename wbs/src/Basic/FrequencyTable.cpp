//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include <xutility>
#include <float.h>


#include "Basic/FrequencyTable.h"
#include "Basic/utilMath.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace WBSF
{

	CFrequency::CFrequency(double low, double high, double delta)
	{
		Reset(low, high, delta);
	}

	void CFrequency::Reset(double low, double high, double delta)
	{
		m_low = low;
		m_high = high;
		m_delta = delta;
		int nbBins = (delta > 0) ? int((high - low) / delta) : 0;
		_ASSERTE(fabs(nbBins*delta - (high - low)) < 0.0001);
		//m_delta = nbBins>0?(high-low)/nbBins:0;

		m_nbValues = 0;
		m_bins.clear();
		if (nbBins > 0)
			m_bins.resize(nbBins);


	}

	const CFrequency& CFrequency::operator += (double v)
	{
		if (m_delta > 0)
		{
			int a = (int)((v - m_low) / m_delta);
			int b = max(0, min(a, int(m_bins.size() - 1)));
			_ASSERTE(b >= 0 && b < (int)m_bins.size());
			m_nbValues++;
			m_bins[b]++;
		}

		return *this;
	}

	int CFrequency::PeakIndex()const
	{
		_ASSERTE(m_bins.size() > 0);

		int peakIndex = -1;
		int nbMax = -1;
		int nbMin = 2147483647;
		for (int i = 0; i<(int)m_bins.size(); i++)
		{
			_ASSERTE(m_bins[i] >= 0);
			if (m_bins[i]>nbMax)
			{
				nbMax = m_bins[i];
				peakIndex = i;
			}

			nbMin = min(nbMin, m_bins[i]);
		}

		//if class is constant: we take the middle of the bins
		if (nbMin == nbMax)
			peakIndex = (int)(m_bins.size() / 2);

		_ASSERTE(peakIndex >= 0 && peakIndex < (int)m_bins.size());
		return peakIndex;
	}

	void CFrequency::RegroupClass(int minimum)
	{
		_ASSERTE(m_bins.size() >= 3);

		int peak = PeakIndex();

		for (int i = 0; i <= peak; i++)
		{
			if (m_bins[i] < minimum)
			{
				m_bins[i + 1] += m_bins[i];
				m_bins[i] = 0;
			}
		}

		for (size_t i = m_bins.size() - 1; (int)i > peak; i--)
		{
			if (m_bins[i] < minimum)
			{
				m_bins[i - 1] += m_bins[i];
				m_bins[i] = 0;
			}
		}

	}

}