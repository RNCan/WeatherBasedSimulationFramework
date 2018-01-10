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
#include "Basic/Shore.h"


using namespace std;


namespace WBSF
{



	CApproximateNearestNeighborPtr CShore::m_pShore;



	ERMsg CShore::SetShore(const std::string& filePath)
	{
		ERMsg msg;

		if (filePath.empty())
		{
			m_pShore.reset();
		}
		else
		{
			ifStream stream;
			msg = stream.open(filePath, std::ios::binary);
			if (msg)
			{
				if (m_pShore.get() == NULL)
					m_pShore = make_shared<CApproximateNearestNeighbor>();

				*m_pShore << stream;
				stream.close();

			}
		}

		return msg;
	}
	
	double CShore::GetShoreDistance(const CGeoPoint& pt)
	{
		double d = 0;
		if (m_pShore)
		{
			CSearchResultVector shorePt;
			CLocation location("", "", pt.m_lat, pt.m_lon);
			if (m_pShore->search(location, 1, shorePt) )
			{
				d = shorePt.front().m_distance;
			}
		}

		return d;
	}

	
}