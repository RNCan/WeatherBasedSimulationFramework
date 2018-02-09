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

		//transfer in CLocation
		//static const double ELEV_FACTOR;
		//static const double SHORE_DISTANCE_FACTOR;

		CSearchResult(size_t index = UNKNOWN_POS, double distance = 0, double deltaElev = 0, double deltaShore = 0, double virtual_distance=0)
		{
			m_index = index;
			m_distance = distance;
			m_deltaElev = deltaElev;
			m_deltaShore = deltaShore;
			m_virtual_distance = virtual_distance;
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
				m_deltaShore = in.m_deltaShore;
				m_virtual_distance = in.m_virtual_distance;
				m_location = in.m_location;

			}

			return *this;
		}


		//warning: only the index is take in the == operator
		bool operator ==(const CSearchResult& in)const	{ return m_index == in.m_index; }
		bool operator !=(const CSearchResult& in)const{ return !operator==(in); }
		bool operator > (const CSearchResult& in)const{ return m_virtual_distance > in.m_virtual_distance; }
		bool operator < (const CSearchResult& in)const{ return m_virtual_distance < in.m_virtual_distance; }


		//return distance in km
		//double GetD(bool bTakeElevation, bool bShoreDistance)const;
		double GetD()const{ return m_virtual_distance; }
			

		double GetXTemp(/*bool bTakeElevation, bool bShoreDistance*/)const
		{
			//double d = GetD(bTakeElevation, bShoreDistance) + 0.0000000001;
			double d = std::max(m_virtual_distance, 0.0000000001);
			return pow(d, -2);
		}

		size_t m_index;
		double m_distance;
		double m_deltaElev;
		double m_deltaShore;
		double m_virtual_distance;
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
			//m_year = YEAR_NOT_INIT;
			//m_filter.reset();
		}

		
		double GetTotalWeight(/*bool bTakeElevation, bool bShoreDistance*/)const
		{
			double totalWeight = 0;

			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
				totalWeight += it->GetXTemp(/*bTakeElevation, bShoreDistance*/);

			return totalWeight;
		}

		std::vector<double> GetStationWeight(/*bool bTakeElevation, bool bShoreDistance*/)const
		{
			std::vector<double>	weight(size());

			double distSum = 0;

			typedef std::pair<CSearchResultVectorBase::const_iterator, std::vector<double>::iterator> FlatSetVectorPairIt;
			for (FlatSetVectorPairIt it(begin(), weight.begin()); it.first != end(); it.first++, it.second++)
			{
				double xtemp = it.first->GetXTemp(/*bTakeElevation, bShoreDistance*/);
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

		double GetWDMean(/*bool bTakeElevation, bool bShoreDistance*/)const
		{
			double WDM = 0;
			double XtmpSum = 0;
			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
			{
				double Xtmp = it->GetXTemp(/*bTakeElevation, bShoreDistance*/);
				WDM += it->GetD(/*bTakeElevation, bShoreDistance*/)*Xtmp;
				XtmpSum += Xtmp;
			}

			WDM /= XtmpSum;
			return WDM;
		}

		double GetDMean(/*bool bTakeElevation, bool bShoreDistance*/)const
		{
			double d = 0;
			for (CSearchResultVectorBase::const_iterator it = begin(); it != end(); it++)
				d += it->GetD(/*bTakeElevation, bShoreDistance*/) / size();

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

		//CWVariables GetFilter()const { return m_filter; }
		//void SetFilter(CWVariables cat){ m_filter = cat; }
		//std::set<int> GetYear()const{return m_years;}

		//int GetYear()const{ return m_year; }
		//void SetYear(int year){ m_year = year; }

		//void SetYear(int year){m_years.insert(year);}
		//void SetYears(const std::set<int>& years, bool bForAllYears){ m_years = year; m_bForAllYears = bForAllYears; }


	protected:

		//CWVariables m_filter;
		//bool bForAllYears;
		//std::set<int> m_years;
		//int m_year;///????
	};

	class CSearchResultSort
	{
	public:

		enum TSortBy{ INDEX, DISTANCE, DELTA_ELEVATION, ABS_DELTA_ELEVATION, DELTA_SHORE, ABS_DELTA_SHORE, VIRTUAL_DISTANCE, WEIGHT, NB_SORTBY };

		CSearchResultSort(TSortBy type = WEIGHT, bool bAscending = false) : m_type(type), m_bAscending(bAscending)
		{}

		//bool operator >(const CSearchResult& results1, const CSearchResult& results2)const
		bool operator ()(const CSearchResult& in1, const CSearchResult& in2)const
		{
			int f = m_bAscending ? 1 : -1;
			bool bRep = false;
			switch (m_type)
			{
			case INDEX:					bRep = f*in1.m_index < f*in2.m_index; break;
			case DISTANCE:				bRep = f*in1.m_distance < f*in2.m_distance; break;
			case DELTA_ELEVATION:		bRep = f*in1.m_deltaElev < f*in2.m_deltaElev; break;
			case ABS_DELTA_ELEVATION:	bRep = f*fabs(in1.m_deltaElev) < f*fabs(in2.m_deltaElev); break;
			case DELTA_SHORE:			bRep = f*in1.m_deltaShore < f*in2.m_deltaShore; break;
			case ABS_DELTA_SHORE:		bRep = f*fabs(in1.m_deltaShore) < f*fabs(in2.m_deltaShore); break;
			case VIRTUAL_DISTANCE:		bRep = f*fabs(in1.m_virtual_distance) < f*fabs(in2.m_virtual_distance); break;
			case WEIGHT:				bRep = f*in1.GetD() < f*in2.GetD(); break;
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
