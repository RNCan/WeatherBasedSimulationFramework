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
//******************************************************************************
#include "stdafx.h"
#include <math.h>


#include "tps.hpp"
#include "Basic/UtilStd.h"
#include "Basic/openMP.h"
#include "Basic/Shore.h"
#include "Geomatic/ThinPlateSpline.h"
#include "KMeanLocal\KMlocal.h" 

using namespace std;

namespace WBSF
{

	typedef tps::Thin_plate_spline_transformation< 2, 1 >  Transformation2;
	typedef tps::Thin_plate_spline_transformation< 3, 1 >  Transformation3;
	typedef tps::Thin_plate_spline_transformation< 4, 1 >  Transformation4;
	typedef tps::Thin_plate_spline_transformation< 5, 1 >  Transformation5;
	typedef tps::Thin_plate_spline_transformation< 6, 1 >  Transformation6;


	//**********************************************************************
	CThinPlateSpline::CThinPlateSpline()
	{
		m_pObject = NULL;
		m_nbDim = 0;
		Reset();

		InitializeCriticalSection(&CS);
	}

	CThinPlateSpline::~CThinPlateSpline()
	{
		DeleteCriticalSection(&CS);
		Reset();
	}

	void CThinPlateSpline::Reset()
	{
		m_inc = 1;

		if (m_pObject)
		{
			switch (m_nbDim)
			{
			case 2: delete (Transformation3*)m_pObject; break;
			case 3: delete (Transformation3*)m_pObject; break;
			case 4: delete (Transformation4*)m_pObject; break;
			case 5: delete (Transformation5*)m_pObject; break;
			case 6: delete (Transformation5*)m_pObject; break;
			default: ASSERT(false);
			}

			m_pObject = NULL;
			m_nbDim = 0;
		}

	}



	ERMsg CThinPlateSpline::Initialization(CCallback& callback)
	{
		ERMsg msg = CGridInterpolBase::Initialization(callback);

		CGridPointVector* pPts = m_pPts.get();

		//double xValPercent = max(0.0, min(1.0, m_param.m_XvalPoints));
		//size_t nbPoints = max(1.0, (1 - xValPercent)*m_pPts->size());
		//m_inc = max(1.0, (double)pPts->size() / nbPoints);
		size_t nbPoints = size_t(pPts->size() / m_inc);


		m_bGeographic = IsGeographic(pPts->GetPrjID());
		m_bUseExposition = m_param.m_bUseExposition && pPts->HaveExposition();
		m_bUseElevation = m_param.m_bUseElevation;
		m_bUseShore = m_param.m_bUseShore;

		m_nbDim = 2;
		if (m_bGeographic)
			m_nbDim++;
		if (m_bUseElevation)
			m_nbDim++;
		if (m_bUseExposition)
			m_nbDim++;
		if (m_bUseShore)
			m_nbDim++;

		std::vector< Transformation2::Domain_point > dp2(m_nbDim == 2 ? nbPoints : 0);
		std::vector< Transformation2::Range_point >  rp2(m_nbDim == 2 ? nbPoints : 0);
		std::vector< Transformation3::Domain_point > dp3(m_nbDim == 3 ? nbPoints : 0);
		std::vector< Transformation3::Range_point >  rp3(m_nbDim == 3 ? nbPoints : 0);
		std::vector< Transformation4::Domain_point > dp4(m_nbDim == 4 ? nbPoints : 0);
		std::vector< Transformation4::Range_point >  rp4(m_nbDim == 4 ? nbPoints : 0);
		std::vector< Transformation5::Domain_point > dp5(m_nbDim == 5 ? nbPoints : 0);
		std::vector< Transformation5::Range_point >  rp5(m_nbDim == 5 ? nbPoints : 0);
		std::vector< Transformation6::Domain_point > dp6(m_nbDim == 6 ? nbPoints : 0);
		std::vector< Transformation6::Range_point >  rp6(m_nbDim == 6 ? nbPoints : 0);

		for (size_t i = 0, ii = 0; ii < pPts->size(); ++i, ii = i * m_inc)
		{
			ASSERT(ii < pPts->size());
			const CGridPoint& ptTmp = pPts->at(ii);
			for (int k = 0, kk = 0; k < 6; k++)
			{
				double dp = VMISS;

				if ((k < 2 && !m_bGeographic) || (k < 3 && m_bGeographic))
				{
					dp = m_bGeographic ? ptTmp(k) : ptTmp[k];
				}
				else if (k == 3 && m_bUseElevation)
				{
					dp = ptTmp.m_elev;
				}
				else if (k == 4 && m_bUseExposition)
				{
					dp = ptTmp.GetExposition();
				}
				else if (k == 5 && m_bUseShore)
				{
					dp = ptTmp.m_shore;
				}

				if (dp != VMISS)
				{
					if (m_nbDim == 2)
						dp2[i][kk] = dp;
					else if (m_nbDim == 3)
						dp3[i][kk] = dp;
					else if (m_nbDim == 4)
						dp4[i][kk] = dp;
					else if (m_nbDim == 5)
						dp5[i][kk] = dp;
					else if (m_nbDim == 6)
						dp6[i][kk] = dp;

					kk++;
				}
			}

			double rp = m_prePostTransfo.Transform(ptTmp.m_event);

			if (m_nbDim == 2)
				rp2[i][0] = rp;
			else if (m_nbDim == 3)
				rp3[i][0] = rp;
			else if (m_nbDim == 4)
				rp4[i][0] = rp;
			else if (m_nbDim == 5)
				rp5[i][0] = rp;
			else if (m_nbDim == 6)
				rp6[i][0] = rp;

		}

		try
		{
			if (m_nbDim == 2)
				m_pObject = (void*) new Transformation2(dp2.begin(), dp2.end(), rp2.begin(), rp2.end(), m_param.m_TPSMaxError);
			else if (m_nbDim == 3)
				m_pObject = (void*) new Transformation3(dp3.begin(), dp3.end(), rp3.begin(), rp3.end(), m_param.m_TPSMaxError);
			else if (m_nbDim == 4)
				m_pObject = (void*) new Transformation4(dp4.begin(), dp4.end(), rp4.begin(), rp4.end(), m_param.m_TPSMaxError);
			else if (m_nbDim == 5)
				m_pObject = (void*) new Transformation5(dp5.begin(), dp5.end(), rp5.begin(), rp5.end(), m_param.m_TPSMaxError);
			else if (m_nbDim == 6)
				m_pObject = (void*) new Transformation6(dp6.begin(), dp6.end(), rp6.begin(), rp6.end(), m_param.m_TPSMaxError);
		}
		catch (std::runtime_error e)
		{
			m_nbDim = 0;
			msg.ajoute("Unable to create Thin Plate Splin. You can try to reduce maximum error");
			msg.ajoute(e.what());
		}
		//}

		return msg;
	}

	//typedef boost::crc_optimal<32, 0x04C11DB7>  fast_crc_type;

	/*int GetCheckSum(const CGridPointResultVector& result)
	{
		boost::crc_32_type crc;
		for (size_t i = 0; i < result.size(); i++)
			crc.process_bytes(&result[i].i, sizeof(result[i].i));

		return crc.checksum();
	}*/

	double CThinPlateSpline::Evaluate(const CGridPoint& pt, int iXval)const
	{
		double value = m_param.m_noData;

		if (iXval >= 0)
		{
			int l = (int)ceil((iXval) / m_inc);
			if (int(l*m_inc) == iXval)
				return value;
		}



		Transformation2::Domain_point  dp2;
		Transformation3::Domain_point  dp3;
		Transformation4::Domain_point  dp4;
		Transformation5::Domain_point  dp5;
		Transformation6::Domain_point  dp6;

		for (int k = 0, kk = 0; k < 6; k++)
		{
			double dp = VMISS;

			if ((k < 2 && !m_bGeographic) || (k < 3 && m_bGeographic))
			{
				dp = m_bGeographic ? pt(k) : pt[k];
			}
			else if (k == 3 && m_bUseElevation)
			{
				dp = pt.m_elev;
			}
			else if (k == 4 && m_bUseExposition)
			{
				dp = pt.GetExposition();
			}
			else if (k == 5 && m_bUseShore)
			{
				dp = pt.m_shore;
			}



			if (dp != VMISS)
			{
				if (m_nbDim == 2)
					dp2[kk] = dp;
				else if (m_nbDim == 3)
					dp3[kk] = dp;
				else if (m_nbDim == 4)
					dp4[kk] = dp;
				else if (m_nbDim == 5)
					dp5[kk] = dp;
				else if (m_nbDim == 6)
					dp6[kk] = dp;

				kk++;
			}
		}


		double rp = 0;
		if (m_nbDim == 2)
			rp = ((Transformation2*)m_pObject)->transform(dp2)[0];
		else if (m_nbDim == 3)
			rp = ((Transformation3*)m_pObject)->transform(dp3)[0];
		else if (m_nbDim == 4)
			rp = ((Transformation4*)m_pObject)->transform(dp4)[0];
		else if (m_nbDim == 5)
			rp = ((Transformation5*)m_pObject)->transform(dp5)[0];
		else if (m_nbDim == 6)
			rp = ((Transformation6*)m_pObject)->transform(dp6)[0];

		
		value = m_prePostTransfo.InvertTransform(rp, m_param.m_noData);


		if ( /*iXval<0 &&*/ m_param.m_bGlobalLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV] || value>m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bGlobalLimitToBound)
					value = min(m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV], max(m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV], value));
				else
					value = m_param.m_noData;
			}
		}

		if ( /*iXval<0 &&*/ m_param.m_bGlobalMinMaxLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_param.m_globalMinLimit || value>m_param.m_globalMaxLimit;
			if (bOutside)
			{
				if (m_param.m_bGlobalMinMaxLimitToBound)
					value = min(m_param.m_globalMaxLimit, max(m_param.m_globalMinLimit, value));
				else
					value = m_param.m_noData;
			}
		}

		return value;

	}



	void CThinPlateSpline::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

	}

	string CThinPlateSpline::GetFeedbackBestParam()const
	{
		string str;
		return str;
	}

	double CThinPlateSpline::GetOptimizedR²(CCallback& callback)const
	{
		return 0;

	}

}