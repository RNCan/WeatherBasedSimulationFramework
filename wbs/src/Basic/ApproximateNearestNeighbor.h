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

#pragma warning( disable : 4244 )
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/access.hpp>

#include "basic/ERMsg.h"
#include "Basic/WeatherDefine.h"
#include "Basic/UtilStd.h"
#include "Basic/Location.h"
#include "Basic/SearchResult.h"

class ANNkd_tree;
typedef double** ANNpointArray;



namespace WBSF
{

	class CSearchResultVector;


	class CApproximateNearestNeighbor
	{
	public:

		friend class boost::serialization::access;


		CApproximateNearestNeighbor();
		CApproximateNearestNeighbor(const CApproximateNearestNeighbor& in);
		~CApproximateNearestNeighbor();

		CApproximateNearestNeighbor& operator=(const CApproximateNearestNeighbor& in);
		std::istream& operator << (std::istream& s);
		std::ostream& operator >> (std::ostream& s)const;
		friend std::ostream& operator << (std::ostream& s, const CApproximateNearestNeighbor& ANN){ return (ANN >> s); }
		friend std::istream& operator >> (std::istream& s, CApproximateNearestNeighbor& ANN){ return (ANN << s); }


		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{

			ar & m_nSize & m_nbDimension & m_positions;
			if (m_pDataPts != NULL)
				ar.save_binary(m_pDataPts[0], m_nbDimension*m_nSize*sizeof(double));
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & m_nSize & m_nbDimension & m_positions;
			if (m_nSize*m_nbDimension > 0)
			{
				m_pDataPts = ::annAllocPts(m_nSize, m_nbDimension);
				ar.load_binary(m_pDataPts[0], m_nbDimension*m_nSize*sizeof(double));
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()


		bool empty()const{ return m_nSize*m_nbDimension == 0; }
		void clear();
		size_t size()const{ return (size_t)m_nSize; }
		void set(const CLocationVector& locations, bool bUseElevation, bool bShoreDistance, const std::vector<__int64>& position = std::vector<__int64>());

		ERMsg search(
			const CLocation& pt,	//point to search
			__int64 nbPoint,			// number of near neighbors
			CSearchResultVector& result,	// result
			double		eps = 0.0)const;	// error bound


		CLocation at(size_t i)const;

		size_t SearchPos2LocPos(size_t i)const{ ASSERT(m_positions.empty() || (i < m_positions.size())); return m_positions.empty() ? i : m_positions[i]; }

	protected:


		void init();

		ANNkd_tree* m_pTreeRoot;
		ANNpointArray m_pDataPts;		// data points
		__int64 m_nbDimension;
		__int64 m_nSize;
		std::vector<__int64> m_positions;

		//temporary storage
		CCriticalSection m_CS;

		//static
		static const __int64 VERSION;
	};


	typedef std::shared_ptr<CApproximateNearestNeighbor> CApproximateNearestNeighborPtr;

}