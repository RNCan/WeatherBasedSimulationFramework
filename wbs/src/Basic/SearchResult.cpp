//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 30-11-2017	Rémi Saint-Amant	Add takeShoreDistance
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-11-2013	Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"
#include <set>

#include "Basic/SearchResult.h"



namespace WBSF
{

	//double CSearchResult::GetD(bool bTakeElevation, bool bShoreDistance)const
	//{
	//	double distance = m_distance;
	//	double deltaElev = fabs(m_deltaElev)*WEATHER::ELEV_FACTOR;
	//	double deltaShore = fabs(m_deltaShore)*WEATHER::SHORE_DISTANCE_FACTOR;
	//	

	//	double d = 0;
	//	if (bTakeElevation&&bShoreDistance)
	//		d = sqrt(Square(distance) + Square(deltaElev) + Square(deltaShore));
	//	else if (bTakeElevation&&!bShoreDistance)
	//		d = sqrt(Square(distance) + Square(deltaElev));
	//	else if (!bTakeElevation&&bShoreDistance)
	//		d = sqrt(Square(distance) + Square(deltaShore));
	//	else
	//		d = distance;

	//	return d/1000;
	//	//return bTakeElevation ? sqrt(Square(distance) + Square(deltaElev)) : distance;
	//}

	void CSearchResultVector::Append(const CSearchResultVector& in, bool bRemoveDuplicate)
	{
		for (CSearchResultVector::const_iterator it = in.begin(); it != in.end(); it++)
		{
			if (!bRemoveDuplicate || std::find(begin(), end(), *it) == end())
				insert(end(), *it);
		}
		sort(begin(), end());
	}



	void CSearchResultVector::Intersect(const CSearchResultVector& in)
	{
		CSearchResultVector me(*this);

		ASSERT(false); //a revoir... très étrnage!!!

		for (CSearchResultVector::const_iterator it = in.begin(); it != in.end(); it++)
		{
			if (find(me.begin(), me.end(), *it) != me.end())
				insert(end(), *it);
		}
	}

	CSearchResultVector& CSearchResultVector::operator |=(const CSearchResultVector& in)
	{
		for (CSearchResultVector::const_iterator it = in.begin(); it != in.end(); it++)
		{
			if (find(begin(), end(), *it) == end())
				insert(end(), *it);
		}

		return *this;
	}

	CSearchResultVector& CSearchResultVector::operator &=(const CSearchResultVector& in)
	{
		const CSearchResultVector& me(*this);

		//remove location NOT inside in
		for (CSearchResultVector::iterator it = begin(); it != end();)
		{
			if (find(begin(), end(), *it) == me.end())
				it = erase(it);
			else
				it++;
		}

		return *this;
	}

	

}