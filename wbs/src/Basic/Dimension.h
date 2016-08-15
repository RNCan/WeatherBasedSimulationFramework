#pragma once

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include "Basic/UtilStd.h"

namespace WBSF
{


	namespace DIMENSION
	{
		enum TDimension{ LOCATION, PARAMETER, REPLICATION, TIME_REF, VARIABLE, NB_DIMENSION };

	}


	class CDimension
	{
	public:

		enum TDimension{ NB_DIMENSION = DIMENSION::NB_DIMENSION };
		static const char * GetDimensionName(size_t d){ ASSERT(d < NB_DIMENSION); return DIMENSION_NAME[d]; }
		static const char * GetDimensionTitle(size_t d);


		CDimension()
		{
			Reset();
		}

		void Reset(long v = 1)
		{
			for (int i = 0; i < NB_DIMENSION; i++)
				m_dimension[i] = v;
		}

		bool operator==(const CDimension& in)const
		{
			bool bEqual = true;
			for (int i = 0; i < NB_DIMENSION; i++)
				bEqual = bEqual && (m_dimension[i] == in.m_dimension[i]);
			return bEqual;
		}
		bool operator!=(const CDimension& in)const{ return !operator==(in); }
		
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_dimension;
		}



		size_t operator[](size_t i)const{ ASSERT(i >= 0 && i < NB_DIMENSION); return m_dimension[i]; }
		size_t& operator[](size_t i){ ASSERT(i >= 0 && i < NB_DIMENSION); return m_dimension[i]; }


	protected:

		boost::array<size_t, NB_DIMENSION> m_dimension;

		static const char* DIMENSION_NAME[NB_DIMENSION];
		static StringVector DIMENSION_TITLE;

	};
}