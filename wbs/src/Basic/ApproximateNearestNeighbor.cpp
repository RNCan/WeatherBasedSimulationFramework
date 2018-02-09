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
#include "Basic/ANN/ANN.h"
#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/UtilStd.h"
#include "Basic/Shore.h"

#include "WeatherBasedSimulationString.h"

using namespace std;


namespace WBSF
{

	double GetShoreDistance(const CLocation& pt)
	{
		double shore = pt.GetShoreDistance();
		//if (shore < 0)
			//shore = CShore::GetShoreDistance(pt);
		return shore;
	}
		
	double GetFactor(size_t d)
	{
		double f = 1;
		if (d == 3)
			f = WEATHER::ELEV_FACTOR;
		else if (d == 4)
			f = WEATHER::SHORE_DISTANCE_FACTOR;
		
		return f;
	}
	//*****************************************************************
	const __int64 CApproximateNearestNeighbor::VERSION = 1;
	CApproximateNearestNeighbor::CApproximateNearestNeighbor()
	{
		m_pTreeRoot = NULL;
		m_pDataPts = NULL;
		m_nSize = 0;
		m_nbDimension = 0;
	}


	CApproximateNearestNeighbor::CApproximateNearestNeighbor(const CApproximateNearestNeighbor& in)
	{
		m_pTreeRoot = NULL;
		m_pDataPts = NULL;
		operator=(in);
	}


	CApproximateNearestNeighbor& CApproximateNearestNeighbor::operator=(const CApproximateNearestNeighbor& in)
	{
		if (&in != this)
		{
			clear();

			m_nbDimension = in.m_nbDimension;
			m_nSize = in.m_nSize;
			if (m_nSize*m_nbDimension > 0)
			{
				m_pDataPts = annAllocPts(m_nSize, m_nbDimension);

				size_t size = m_nSize*m_nbDimension*sizeof(ANNcoord);
				memcpy_s(m_pDataPts[0], size, in.m_pDataPts[0], size);
				m_positions = in.m_positions;
			}
		}

		return *this;
	}



	CApproximateNearestNeighbor::~CApproximateNearestNeighbor()
	{
		clear();
	}


	void CApproximateNearestNeighbor::clear()
	{
		m_CS.Enter();

		if (m_pTreeRoot)
		{
			delete m_pTreeRoot;
			m_pTreeRoot = NULL;
			//annClose();//delete global variable: in multi-thread is not a good idea
		}

		if (m_pDataPts)
		{
			annDeallocPts(m_pDataPts);
			m_pDataPts = NULL;
		}

		m_nSize = 0;
		m_nbDimension = 0;
		m_positions.clear();

		m_CS.Leave();
	}

	ostream& CApproximateNearestNeighbor::operator >> (ostream& stream)const
	{
		assert(m_nbDimension == 3 || m_nbDimension == 4 || m_nbDimension == 5);
		stream.write((char*)&VERSION, sizeof(VERSION));
		stream.write((char*)&m_nSize, sizeof(m_nSize));
		stream.write((char*)&m_nbDimension, sizeof(m_nbDimension));
		if (m_pDataPts)
			stream.write((char*)(&(m_pDataPts[0][0])), m_nSize*m_nbDimension*sizeof(ANNcoord));//used adress of point vector

		stream << (__int64)m_positions.size();
		if (!m_positions.empty())
			stream.write((char*)&(m_positions[0]), m_positions.size()*sizeof(__int64));

		return stream;
	}


	istream& CApproximateNearestNeighbor::operator << (istream& stream)
	{
		clear();

		__int64 version = 0;
		stream.read((char*)&version, sizeof(version));
		ASSERT(version == VERSION);

		stream.read((char*)&m_nSize, sizeof(m_nSize));
		stream.read((char*)&m_nbDimension, sizeof(m_nbDimension)); assert(m_nbDimension == 3 || m_nbDimension == 4 || m_nbDimension == 5);
		if (m_nSize*m_nbDimension > 0)
		{
			m_pDataPts = annAllocPts(m_nSize, m_nbDimension);
			stream.read((char*)(&(m_pDataPts[0][0])), m_nSize*m_nbDimension*sizeof(ANNcoord));//used adress of point vector
		}

		__int64 posSize = 0;
		stream >> posSize;
		m_positions.resize(posSize);
		if (!m_positions.empty())
			stream.read((char*)&(m_positions[0]), posSize*sizeof(__int64));

		return stream;
	}

	void CApproximateNearestNeighbor::set(const CLocationVector& locations, bool bUseElevation, bool bUseShoreDistance, const vector<__int64>& positions)
	{
		ASSERT(positions.empty() || positions.size() == locations.size());

		clear();

		m_nbDimension = (bUseShoreDistance &&WEATHER::SHORE_DISTANCE_FACTOR > 0) ? 5 : bUseElevation ? 4 : 3;
		//m_nbDimension = bUseElevation ? 4 : 3;
		
		m_nSize = locations.size();
		if (m_nbDimension*m_nSize > 0)
			m_pDataPts = annAllocPts(m_nSize, m_nbDimension);

		m_positions = positions;

		size_t i = 0;
		for (CLocationVector::const_iterator it = locations.begin(); it != locations.end(); it++, i++)
		{
			for (__int64 d = 0; d < m_nbDimension; d++)
				m_pDataPts[i][d] = it->GetGeocentricCoord(d)*GetFactor(d);
		}
	}

	CLocation CApproximateNearestNeighbor::at(size_t i)const
	{
		CLocation pt;
		pt.SetXY(m_pDataPts[i][0], m_pDataPts[i][1], m_pDataPts[i][2]);
		if (m_nbDimension >= 4)
			pt.m_z = m_pDataPts[i][3] / WEATHER::ELEV_FACTOR;
		if (m_nbDimension >= 5)
			pt.SetShoreDistance(m_pDataPts[i][4] / WEATHER::SHORE_DISTANCE_FACTOR);

		return pt;
	}



	void CApproximateNearestNeighbor::init()
	{
		m_CS.Enter();
		if (m_pTreeRoot == NULL)
		{

			if (m_pTreeRoot)
			{
				delete m_pTreeRoot;
				m_pTreeRoot = NULL;
			}

			if (m_nSize*m_nbDimension > 0)
			{
				ASSERT(m_pDataPts);
				m_pTreeRoot = (ANNkd_tree*)new ANNkd_tree(m_pDataPts, m_nSize, m_nbDimension);
			}
		}
		m_CS.Leave();
	}

	ERMsg CApproximateNearestNeighbor::search(const CLocation& pt, __int64 nbPoint, CSearchResultVector& result, double eps)const
	{
		ERMsg message;

		CApproximateNearestNeighbor& me = const_cast<CApproximateNearestNeighbor&>(*this);
		me.init();

		result.clear();

		//We make the search event if they are not enough points
		__int64 nbPointSearch = min(nbPoint, m_nSize);
		ASSERT(nbPointSearch <= m_nSize);

		if (m_nSize > 0)
		{
			ASSERT(m_nbDimension >= 3 && m_nbDimension <= 5);

			result.resize(nbPointSearch);

			ANNidxArray	nn_idx = new ANNidx[nbPointSearch];		// allocate near neigh indices
			ANNdistArray	dd = new ANNdist[nbPointSearch];	// allocate near neighbor dists
			ANNpoint	q = annAllocPt(m_nbDimension);			// query point

			for (__int64 d = 0; d < m_nbDimension; d++)
				q[d] = pt.GetGeocentricCoord(d)*GetFactor(d);//elevation have scaled by 100

			m_pTreeRoot->annPkSearch(q, nbPointSearch, nn_idx, dd, eps);
			for (__int64 i = 0; i < nbPointSearch; i++)
			{
				

				size_t index = nn_idx[i];
				result[i].m_index = SearchPos2LocPos(index);
				//result[i].m_distance = sqrt(dd[i]);
				result[i].m_location.SetXY(m_pDataPts[index][0], m_pDataPts[index][1], m_pDataPts[index][2]);
				result[i].m_distance = pt.GetDistance(result[i].m_location, false, false);
				
				if (m_nbDimension == 3)
				{
					static const double _2x6371x1000_ = 2 * 6371 * 1000;
					double test = _2x6371x1000_*asin( sqrt(dd[i]) / _2x6371x1000_);//apply sphere curve to distance
					int uu;
					uu = 0;
					//result[i].m_distance = _2x6371x1000_*asin(result[i].m_distance / _2x6371x1000_);//apply sphere curve to distance

				}
				

				
				if (m_nbDimension >= 4)
				{
					result[i].m_location.m_z = m_pDataPts[index][3] / WEATHER::ELEV_FACTOR;
					result[i].m_deltaElev = result[i].m_location.m_z - pt.m_z;
				}

				if (m_nbDimension >= 5)
				{
					//result[i].m_location.m_z = m_pDataPts[index][3] / ELEV_FACTOR;
					double shore = m_pDataPts[index][4] / WEATHER::SHORE_DISTANCE_FACTOR;
					result[i].m_location.SetShoreDistance(shore);
					result[i].m_deltaShore = shore - GetShoreDistance(pt);
				}

				//compute virtual distance only after setted elevation and shore distance
				result[i].m_virtual_distance = pt.GetDistance(result[i].m_location, m_nbDimension >= 4, m_nbDimension >= 5);
			}

			annDeallocPt(q);
			delete[] nn_idx;
			delete[] dd;

			ASSERT(result.size() == nbPointSearch);
		}

		if (result.size() != nbPoint)
		{
			message.ajoute(FormatMsg(IDS_WG_NOT_ENAUGHT_POINTS, to_string(result.size()), to_string(nbPoint)));
		}

		return message;
	}

}//namespace WBSF