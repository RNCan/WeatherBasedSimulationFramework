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
// 15-11-2013	Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"
#include <set>

#include "Basic/SearchResult.h"



namespace WBSF
{

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

		m_filter.reset();
		m_year = YEAR_NOT_INIT;

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

		m_filter.reset();
		m_year = YEAR_NOT_INIT;

		return *this;
	}

	/*
	void CSearchResultVector::GetWCentroid(const CLocation& st, CLocation& stCentro)
	{
	//CStatistic centroid[3];
	double lat=0;
	double lon=0;
	double elev=0;
	//for(int i=0; i<index.size(); i++)
	//{
	//}
	double WDM=0;
	double XtmpSum = 0;
	for(int i=0; i<size(); i++)
	{
	double Xtmp = at(i).GetXTemp(true);
	XtmpSum += Xtmp;

	lat += (at(i).m_st.GetLat())*Xtmp;
	lon += (at(i).m_st.GetLon())*Xtmp;
	elev += (at(i).m_st.GetElev())*Xtmp;

	}

	lat/=XtmpSum;
	lon/=XtmpSum;
	elev/=XtmpSum;

	stCentro.SetName("WCentroid");
	stCentro.SetLat(lat);
	stCentro.SetLon(lon);
	stCentro.SetElev(int(elev));
	}
	*/

}