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
#include <math.h>
#include <sstream>

#include "Basic/SimulatedAnnealing.h"

using namespace std;


namespace WBSF
{

	//*****************************************************************
	const char* CSAControl::XML_FLAG = "Control";
	const char* CSAControl::MEMBER_NAME[NB_MEMBER] = { "OptimisationType", "StatisticType", "InitialTemperature", "ReductionFactor", "ReductionFactor2", "Epsilon", "NbCycles", "NbIterations",  "NbEpsilons", "MaxEvaluations", "Seed1", "Seed2", "nbSkipLoop"};


	CSAControl::CSAControl()
	{
		Reset();
	}

	CSAControl::~CSAControl()
	{
	}


	CSAControl::CSAControl(const CSAControl& in)
	{
		operator=(in);
	}

	void CSAControl::Reset()
	{
		m_bMax = false;
		m_RT = 0.85;
		m_RT2 = 1.0;
		m_EPS = 0.001;
		m_NS = 20;
		m_NT = 10;
		m_NEPS = 4;
		m_MAXEVL = 1000000;
		m_seed1 = -1;
		m_seed2 = -1;
		m_T = 100;
		m_statisticType = RSS;
		m_missing = DBL_MAX;
		m_nbSkipLoop = 0;
	}

	CSAControl& CSAControl::operator=(const CSAControl& in)
	{
		if (&in != this)
		{
			m_bMax = in.m_bMax;
			m_RT = in.m_RT;
			m_RT2 = in.m_RT2;
			m_EPS = in.m_EPS;
			m_NS = in.m_NS;
			m_NT = in.m_NT;
			m_NEPS = in.m_NEPS;
			m_MAXEVL = in.m_MAXEVL;
			m_seed1 = in.m_seed1;
			m_seed2 = in.m_seed2;
			m_T = in.m_T;
			m_statisticType = in.m_statisticType;
			m_nbSkipLoop = in.m_nbSkipLoop;
		}

		ASSERT(in == *this);

		return *this;
	}

	bool CSAControl::operator == (const CSAControl& in)const
	{
		bool bEqual = true;

		if (m_bMax != in.m_bMax) bEqual = false;
		if (fabs(m_T - in.m_T) > 0.001) bEqual = false;
		if (fabs(m_RT - in.m_RT) > 0.001) bEqual = false;
		if (fabs(m_RT2 - in.m_RT2) > 0.001) bEqual = false;
		if (fabs(m_EPS - in.m_EPS) > 0.00000001) bEqual = false;
		if (m_NS != in.m_NS) bEqual = false;
		if (m_NT != in.m_NT) bEqual = false;

		if (m_NEPS != in.m_NEPS) bEqual = false;
		if (m_MAXEVL != in.m_MAXEVL) bEqual = false;
		if (m_seed1 != in.m_seed1) bEqual = false;
		if (m_seed2 != in.m_seed2) bEqual = false;
		if (m_statisticType != in.m_statisticType) bEqual = false;
		if (m_nbSkipLoop != in.m_nbSkipLoop) bEqual = false;

		return bEqual;
	}



	std::string CSAControl::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		std::string str;
		switch (i)
		{
		case TYPE_OPTIMISATION:str = ToString(m_bMax); break;
		case STAT_OPTIMISATION: str = ToString(m_statisticType); break;
		case INITIAL_TEMPERATURE:str = ToString(m_T, -1); break;
		case REDUCTION_FACTOR:str = ToString(m_RT); break;
		case REDUCTION_FACTOR2:str = ToString(m_RT2); break;
		case EPSILON:str = ToString(m_EPS, -1); break;
		case NB_CYCLE:str = ToString(m_NS); break;
		case NB_SKIP_LOOP:str = ToString(m_nbSkipLoop); break;
		case NB_ITERATION:str = ToString(m_NT); break;
		case NB_EPSILON:str = ToString(m_NEPS); break;
		case MAX_EVALUATION:str = ToString(m_MAXEVL); break;
		case SEED1:str = ToString(m_seed1); break;
		case SEED2:str = ToString(m_seed2); break;

		default: ASSERT(false);
		}

		return str;
	}

	void CSAControl::SetMember(size_t i, const std::string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);
		switch (i)
		{
		case TYPE_OPTIMISATION: m_bMax = ToBool(str); break;
		case STAT_OPTIMISATION: m_statisticType = ToInt(str); break;
		case INITIAL_TEMPERATURE:m_T = ToDouble(str); break;
		case REDUCTION_FACTOR:m_RT = ToDouble(str); break;
		case REDUCTION_FACTOR2:m_RT2 = ToDouble(str); break;
		case EPSILON: m_EPS = ToDouble(str); break;
		case NB_CYCLE: m_NS = ToInt(str); break;
		case NB_SKIP_LOOP: m_nbSkipLoop = ToInt(str); break;
		case NB_ITERATION:m_NT = ToInt(str); break;
		case NB_EPSILON:m_NEPS = ToInt(str); break;
		case MAX_EVALUATION:m_MAXEVL = ToInt(str); break;
		case SEED1:m_seed1 = ToInt(str); break;
		case SEED2:m_seed2 = ToInt(str); break;
		default: ASSERT(false);
		}
	}

	//*****************************************************************

	const char* CSAParameter::XML_FLAG = "Parameter";
	const char* CSAParameter::MEMBER_NAME[NB_MEMBER] = { "Name", "Value", "Bound" };
	CSAParameter::CSAParameter(string name, double value, double low, double hi)
	{
		m_name = name;
		m_initialValue = value;
		m_bounds.SetBound(low, hi);

	}
	CSAParameter::~CSAParameter()
	{}

	CSAParameter::CSAParameter(const CSAParameter& in)
	{
		operator=(in);
	}

	void CSAParameter::Reset()
	{
		m_name.clear();
		m_initialValue = 0;
		m_bounds.Reset();
	}

	CSAParameter& CSAParameter::operator =(const CSAParameter& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			m_initialValue = in.m_initialValue;
			m_bounds = in.m_bounds;
		}

		ASSERT(in == *this);
		return *this;
	}

	bool CSAParameter::operator == (const CSAParameter& in)const
	{
		bool bEqual = true;

		if (m_name != in.m_name) bEqual = false;
		if (m_initialValue != in.m_initialValue)bEqual = false;
		if (m_bounds != in.m_bounds)bEqual = false;

		return bEqual;
	}

}