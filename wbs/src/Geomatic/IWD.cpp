//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//http://www.ems-i.com/smshelp/Data_Module/Interpolation/Inverse_Distance_Weighted.htm
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************



#include "stdafx.h"
#include "Basic/UtilMath.h"
#include "Basic/Statistic.h"
#include "Geomatic/IWD.h"

using namespace std;

namespace WBSF
{
	

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CIWD::CIWD()
	{}

	CIWD::~CIWD()
	{}

	ERMsg CIWD::Initialization(CCallback& callback)
	{
		ERMsg msg = CGridInterpolBase::Initialization(callback);
		
		if(!m_bInit)
		{
			//m_lastCheckSum = checkSum;
			ASSERT(!m_pPts->empty());
			
			m_pANNSearch = make_unique<CANNSearch>();
			m_pANNSearch->Init(m_pPts, m_param.m_bUseElevation, m_param.m_bUseShore);
		

			m_bInit = true;
		}

		return msg;
	}

	double CIWD::GetIWD(const CGridPointResultVector& result)const
	{
		CStatistic W;
		CStatistic H;
		double R = max(0.000001, result.back().d);
		for (size_t i = 0; i < result.size(); i++)
		{
			double Fi = m_prePostTransfo.Transform(m_pPts->at(result[i].i).m_event);
			double Hi = max(0.000001, result[i].d);
			double Wi = 0;
			switch (m_param.m_IWDModel)
			{
			case CGridInterpolParam::IWD_CLASIC: Wi = pow(Hi, -m_param.m_power); break;
			case CGridInterpolParam::IWD_TENSION: Wi = pow((R - Hi) / (R*Hi), m_param.m_power); break;
			default: _ASSERTE(false);
			}

			H += Wi*Fi;
			W += Wi;
		}

		return m_prePostTransfo.InvertTransform(H[SUM] / W[SUM], m_param.m_noData);
	}

	void CIWD::Search(CGridPointResultVector& result, const CGridPoint& pt, int iXval)const
	{
		ASSERT(m_pANNSearch->GetSize() > 0);

		ERMsg msg;

		//Add one point if we are in X validation
		size_t nbPoint = m_pANNSearch->GetSize() - (iXval >= 0 ? 1 : 0);
		if (m_param.m_nbPoints > 0)
			nbPoint = min(m_pANNSearch->GetSize(), m_param.m_nbPoints + size_t(iXval >= 0 ? 1ull : 0ull));

		m_pANNSearch->Search(pt, nbPoint, result);
		if (iXval >= 0)
		{
			ASSERT(result.empty() || result[0].d < 0.1);
			for (size_t i = 0; i < result.size(); i++)
			{
				if (result[i].i == iXval)
				{
					result.erase(result.begin() + i);
					break;
				}
			}

		}
	}

	double CIWD::Evaluate(const CGridPoint& pt, int iXval)const
	{
		if (iXval >= 0)
		{
			int l = (int)ceil((iXval) / m_inc);
			if (int(l*m_inc) == iXval)
				return m_param.m_noData;
		}

		//Add one point if we are in X validation
		size_t nbPoint = m_pANNSearch->GetSize() - (iXval >= 0 ? 1 : 0);
		if (m_param.m_nbPoints > 0)
			nbPoint = min(nbPoint, m_param.m_nbPoints + size_t(iXval >= 0 ? 1ull : 0ull));


		CGridPointResultVector result;
		m_pANNSearch->Search(pt, nbPoint, result);
		if (iXval >= 0)
		{
			ASSERT(result.empty() || result[0].d < 0.1);
			for (size_t i = 0; i < result.size(); i++)
			{
				if (result[i].i == iXval)
				{
					result.erase(result.begin() + i);
					break;
				}
			}

		}
		else
		{
			if (result.empty() || result[0].d > m_param.m_maxDistance)
				return m_param.m_noData;
		}


		double value = GetIWD(result);
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

	string CIWD::GetFeedbackBestParam()const
	{
		
		string str;
		int varPrint[2] = { CGridInterpolParam::IWD_MODEL, CGridInterpolParam::IWD_POWER };
		for (int i = 0; i < 2; i++)
		{
			str += string(CGridInterpolParam::GetMemberName(varPrint[i])) + " = " + m_param.GetMember(varPrint[i])+"\n";
		}

		return str;
	};

	void CIWD::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

		int nbModel = m_param.m_IWDModel == CGridInterpolParam::BEST_IWD_MODEL ? CGridInterpolParam::NB_IWD_MODEL : 1;
		int nbPower = m_param.m_power <= 0 ? 6 : 1;
		if (nbModel*nbPower > 1)
		{
			parameterset.resize(nbModel*nbPower, m_param);
			for (int i = 0; i < nbModel; i++)
			{
				for (int j = 0; j < nbPower; j++)
				{
					if (nbModel>1)
						parameterset[i*nbPower + j].m_IWDModel = i;

					if (nbPower>1)
						parameterset[i*nbPower + j].m_power = 1 + 0.5*j;
				}
			}
		}
	}



}