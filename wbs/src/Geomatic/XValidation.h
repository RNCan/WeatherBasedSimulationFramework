//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/GeoBasic.h"
#include "Basic/UtilMath.h"

namespace WBSF
{
	class CXValElem
	{
	public:

		double m_observed;
		double m_predicted;

		CXValElem(double observed = 0, double predicted = 0)
		{
			m_observed = observed;
			m_predicted = predicted;
		}

		bool operator == (const CXValElem& in)const
		{
			return fabs(m_observed - in.m_observed) < EPSILON_DATA &&
				fabs(m_predicted - in.m_predicted) < EPSILON_DATA;
		};
		bool operator != (const CXValElem& in)const{ return !operator==(in); };

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_observed & m_predicted;
		}

		//friend boost::serialization::access;
	};

	typedef std::vector <CXValElem> CXValElemVector;


	class CXValidation : public CXValElemVector
	{

	public:

		std::string m_interpolMethodName;
		double		m_R2;
		double		m_intercept;
		double		m_slope;
		double		m_S;
		double		m_t;
		double		m_MSE;


		CXValidation(){ clear(); }
		CXValidation(const CXValidation& in){ operator=(in); }

		void Reset(){ clear(); }
		void CXValidation::clear()
		{
			clear();
			m_interpolMethodName.clear();
			m_R2 = 0;
			m_intercept = 0;
			m_slope = 0;
			m_S = 0;
			m_t = 0;
			m_MSE = 0;
		}


		CXValidation& operator =(const CXValidation& in);
		bool operator == (const CXValidation& in)const;
		bool operator != (const CXValidation& in)const{ return !operator==(in); }

		

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_interpolMethodName & m_R2 & m_intercept & m_slope & m_S & m_t & m_MSE & boost::serialization::base_object<CXValElemVector>(*this);
		}



	};



}