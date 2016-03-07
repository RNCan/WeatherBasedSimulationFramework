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
#include "basic/ERMsg.h"

#include "Basic/UtilMath.h"
#include "Basic/WeatherDefine.h"
#include "Basic/Location.h"
#include "Basic/UtilStd.h"

namespace WBSF
{
	class CSearchResult
	{
	public:

		CSearchResult(size_t index = UNKNOWN_POS, double distance = 0, double deltaElev = 0)
		{
			m_index = index;
			m_distance = distance;
			m_deltaElev = deltaElev;
		}
		CSearchResult(const CSearchResult& in)
		{
			operator=(in);
		}

		CSearchResult& operator=(const CSearchResult& in)
		{
			if (&in != this)
			{
				m_index = in.m_index;
				m_distance = in.m_distance;
				m_deltaElev = in.m_deltaElev;
				m_location = in.m_location;
			}

			return *this;
		}


		//warning: only the index is take in the == operator
		bool operator ==(const CSearchResult& in)const	{ return m_index == in.m_index; }
		bool operator !=(const CSearchResult& in)const{ return !operator==(in); }
		bool operator > (const CSearchResult& in)const{ return GetD(true) > in.GetD(true); }
		bool operator < (const CSearchResult& in)const{ return GetD(true) < in.GetD(true); }


		//return distance in km
		double GetD(bool bTakeElevation)const
		{
			double deltaElev = fabs(m_deltaElev) / 10;
			double distance = m_distance / 1000;
			return bTakeElevation ? sqrt(Square(distance) + Square(deltaElev)) : distance;
		}

		double GetXTemp(bool bTakeElevation)const
		{
			double d = GetD(bTakeElevation) + 0.0000000001;
			return pow(d, -2);
		}

		size_t m_index;
		double m_distance;
		double m_deltaElev;
		CLocation m_location;
	};


	typedef std::vector<CSearchResult> CSearchResultVectorBase;
	class CSearchResultVector : public CSearchResultVectorBase
	{
	public:

		CSearchResultVector()
		{
			Reset();
		}

		
		void Reset()
		{
			clear();
			m_year = YEAR_NOT_INIT;
			m_filter.reset();
		}

		
		double GetTotalWeight(bool bTakeElevation = true)const
		{
			double totalWeight = 0;

			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
				totalWeight += it->GetXTemp(bTakeElevation);

			return totalWeight;
		}

		std::vector<double> GetStationWeight(bool bTakeElevation = true)const
		{
			std::vector<double>	weight(size());

			double distSum = 0;

			typedef std::pair<CSearchResultVectorBase::const_iterator, std::vector<double>::iterator> FlatSetVectorPairIt;
			for (FlatSetVectorPairIt it(begin(), weight.begin()); it.first != end(); it.first++, it.second++)
			{
				double xtemp = it.first->GetXTemp(bTakeElevation);
				*(it.second) = xtemp;
				distSum += xtemp;
			}

			for (std::vector<double>::iterator it = weight.begin(); it != weight.end(); it++)
				*it /= distSum;

			return weight;
		}

		void Append(const CSearchResultVector& in, bool bRemoveDuplicate = true);
		CSearchResultVector& operator |=(const CSearchResultVector& in);
		CSearchResultVector& operator &=(const CSearchResultVector& in);

		//keep only 
		void Intersect(const CSearchResultVector& in);

		double GetWDMean(bool bTakeElevation = true)const
		{
			double WDM = 0;
			double XtmpSum = 0;
			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
			{
				double Xtmp = it->GetXTemp(bTakeElevation);
				WDM += it->GetD(bTakeElevation)*Xtmp;
				XtmpSum += Xtmp;
			}

			WDM /= XtmpSum;
			return WDM;
		}

		double GetDMean(bool bTakeElevation = true)const
		{
			double d = 0;
			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
				d += it->GetD(bTakeElevation) / size();

			return d;
		}

		double GetDeltaElevMean(bool bAbsolute)const
		{
			double d = 0;
			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
				d += bAbsolute ? fabs(it->m_deltaElev) : it->m_deltaElev;

			d /= size();
			return d;
		}

		void GetCentroid(CLocation& stCentro)const
		{
			std::vector<size_t> index(size());
			for (size_t i = 0; i < size(); i++)
				index[i] = i;

			GetCentroid(stCentro, index);
		}
		void GetCentroid(CLocation& stCentro, const std::vector<size_t>& index)const
		{
			stCentro.Reset();
			double lat = 0;
			double lon = 0;
			double elev = 0;
			for (size_t i = 0; i < index.size(); i++)
			{
				lat += at(index[i]).m_location.m_lat;
				lon += at(index[i]).m_location.m_lon;
				elev += at(index[i]).m_location.m_elev;
			}

			stCentro.m_name = "centroid";
			stCentro.m_ID = "centroid";
			stCentro.m_lat = lat / index.size();
			stCentro.m_lon = lon / index.size();
			stCentro.m_elev = elev / index.size();
		}

		CWVariables GetFilter()const { return m_filter; }
		void SetFilter(CWVariables cat){ m_filter = cat; }
		//std::set<int> GetYear()const{return m_years;}

		int GetYear()const{ return m_year; }
		void SetYear(int year){ m_year = year; }

		//void SetYear(int year){m_years.insert(year);}
		//void SetYears(const std::set<int>& years, bool bForAllYears){ m_years = year; m_bForAllYears = bForAllYears; }


	protected:

		CWVariables m_filter;
		//bool bForAllYears;
		//std::set<int> m_years;
		int m_year;///????
	};

	class CSearchResultSort
	{
	public:

		enum TSortBy{ INDEX, DISTANCE, DISTANCE_ELEV, WEIGHT = DISTANCE_ELEV, ELEVATION, ELEVATION_ABS, NB_SORTBY };

		CSearchResultSort(TSortBy type = DISTANCE_ELEV, bool bAscending = false) : m_type(type), m_bAscending(bAscending)
		{}

		//bool operator >(const CSearchResult& results1, const CSearchResult& results2)const
		bool operator ()(const CSearchResult& in1, const CSearchResult& in2)const
		{
			int f = m_bAscending ? 1 : -1;
			bool bRep = false;
			switch (m_type)
			{
			case INDEX:			bRep = f*in1.m_index > f*in2.m_index; break;
			case DISTANCE:		bRep = f*in1.m_distance > f*in2.m_distance; break;
			case DISTANCE_ELEV: bRep = f*in1.GetD(true) > f*in2.GetD(true); break;
			case ELEVATION:		bRep = f*in1.m_deltaElev > f*in2.m_deltaElev; break;
			case ELEVATION_ABS:	bRep = f*fabs(in1.m_deltaElev) > f*fabs(in2.m_deltaElev); break;
			default: ASSERT(false);
			}
			return bRep;

		}

	protected:

		TSortBy m_type;
		bool m_bAscending;

	};

	typedef std::array<CSearchResultVector, HOURLY_DATA::NB_VAR_H> CSearchArray;

}//namespace WBSF
