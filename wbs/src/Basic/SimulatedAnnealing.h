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


#include "basic/ERMsg.h"
#include "basic/Statistic.h"
#include "Basic/zenXml.h"



namespace WBSF
{

	//**********************************************************************
	//CSAControl
	class CSAControl
	{
	public:
		enum TMember { TYPE_OPTIMISATION, STAT_OPTIMISATION, INITIAL_TEMPERATURE, REDUCTION_FACTOR, EPSILON, NB_CYCLE, NB_ITERATION, NB_EPSILON, MAX_EVALUATION, SEED1, SEED2, NB_MEMBER };

		static const char* GetMemberName(int i) { _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag() { return XML_FLAG; }

		CSAControl();
		~CSAControl();
		CSAControl(const CSAControl& ctrl);
		void Reset();

		CSAControl& operator=(const CSAControl& ctrl);
		bool operator == (const CSAControl& in)const;
		bool operator != (const CSAControl& in)const { return !operator==(in); }

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		//void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }


		double AdjustFValue(double F)
		{
			if (!m_bMax)
				F = -F;

			return F;
		}

		double GetFinalFValue(const CStatisticXY& stat)
		{
			ASSERT(m_statisticType< NB_STATXY_TYPE);
			return AdjustFValue(stat[m_statisticType]);
		}

		double GetFinalFValue(const CStatisticXYEx& stat)
		{
			ASSERT(m_statisticType < NB_STATXY_TYPE_EX);
			return AdjustFValue(stat[m_statisticType]);
		}

		bool Max()const { return m_bMax; }

		void SetMax(bool bMax) { m_bMax = bMax; }

		double RT()const { return m_RT; }
		void SetRT(double RT) { m_RT = RT; }
		double EPS()const { return m_EPS; }
		void SetEPS(double EPS) { m_EPS = EPS; }
		//double deltaVar()const{	return m_deltaVar; }
		//void SetdeltaVar(double EPS)	{ m_deltaVar = EPS; }
		long NS()const { return m_NS; }
		void SetNS(long NS) { m_NS = NS; }
		long NT()const { return m_NT; }
		void SetNT(long NT) { m_NT = NT; }
		long NEPS()const { return m_NEPS; }
		void SetNEPS(long NEPS) { m_NEPS = NEPS; }
		long MAXEVL()const { return m_MAXEVL; }
		void SetMAXEVL(long MAXEVL) { m_MAXEVL = MAXEVL; }
		long Seed1()const { return m_seed1; }
		void SetSeed1(long seed1) { m_seed1 = seed1; }
		long Seed2()const { return m_seed2; }
		void SetSeed2(long seed2) { m_seed2 = seed2; }
		double T()const { return m_T; }
		void SetT(double T) { _ASSERTE(T > 0.0);  m_T = T; }

		//void LoadProfile(const std::string& section);
		//void SaveProfile(const std::string& section);

		double GetVMiss() { return m_missing; }
		void SetVMiss(double in) { m_missing = in; }

		short m_statisticType;
		bool m_bMax;
		double m_RT;
		double m_EPS;
		long m_NS;
		long m_NT;
		long m_NEPS;
		long m_MAXEVL;
		long m_seed1;
		long m_seed2;
		double m_T;


	protected:

		//internal value
		double m_missing;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};





	//*******************************************************************************
	//CVariableBound
	class CVariableBound
	{
	public:

		CVariableBound(double low = -1.0E25, double hi = 1.0E25) { m_lowerBound = low, m_upperBound = hi; }

		void Reset() { m_lowerBound = -1.0E25; m_upperBound = 1.0E25; }
		bool operator == (const CVariableBound& in)const
		{
			return	fabs(m_lowerBound - in.m_lowerBound) < 0.0000001 &&
				fabs(m_upperBound - in.m_upperBound) < 0.0000001;
		}
		bool operator != (const CVariableBound& in)const { return !operator==(in); }


		bool IsOutOfBound(double& value)const { return (value < m_lowerBound) || (value > m_upperBound); }
		double GetExtent()const
		{
			_ASSERTE(m_upperBound >= m_lowerBound);
			return m_upperBound - m_lowerBound;
		}

		double GetLowerBound()const { return m_lowerBound; }
		double GetUpperBound()const { return m_upperBound; }

		void SetBound(double lowerBound, double upperBound)
		{
			m_lowerBound = std::min(lowerBound, upperBound);
			m_upperBound = std::max(lowerBound, upperBound);
			_ASSERTE(m_lowerBound <= m_upperBound);
		}

		std::string ToString()const
		{
			return FormatA("[%.7g, %.7g]", m_lowerBound, m_upperBound);
		}

		void FromString(const std::string& str)
		{
			sscanf(str.c_str(), "[%lf, %lf]", &m_lowerBound, &m_upperBound);
		}


		double m_lowerBound;
		double m_upperBound;

	};





	typedef std::vector< CVariableBound> CVariableBoundVector;

	//**********************************************************************
	//CSAParam
	class CSAParameter
	{
	public:
		enum TMember { NAME, VALUE, BOUND, NB_MEMBER };

		static const char* GetMemberName(int i) { _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag() { return XML_FLAG; }

		CSAParameter(std::string name = "", double value = 0, double lo = -1.0E25, double hi = 1.0E25);
		~CSAParameter();
		CSAParameter(const CSAParameter& in);

		void Reset();
		CSAParameter& operator =(const CSAParameter& in);
		bool operator == (const CSAParameter& in)const;
		bool operator != (const CSAParameter& in)const { return !operator==(in); }
		std::string to_string()const { return m_name + "=" + FormatA("%.7g", m_initialValue) + m_bounds.ToString(); }
		CSAParameter& from_string(const std::string& in) { return operator=(FROM_STRING(in)); }
		CSAParameter FROM_STRING(const std::string& in)
		{
			StringVector tmp(in, "=[,]|");
			if (tmp.size() == 4)
				return CSAParameter(tmp[0], stod(tmp[1]), stod(tmp[2]), stod(tmp[3]));

			return CSAParameter();
		}

		std::string m_name;
		double m_initialValue;
		CVariableBound m_bounds;

	protected:

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};



	typedef std::vector< CSAParameter > CSAParameterVector;
	typedef std::map<std::string, CSAParameterVector> CSAParametersMap;





	//*******************************************************************************
	//CComputationVariable
	class CComputationVariable
	{
	public:
		CComputationVariable() { Initialize(1, 4, 0); }
		void PrepareForAnotherLoop(double RT)
		{
			m_T = RT * m_T;
			for (size_t I = m_FSTAR.size() - 1; I > 0; I--)
			{
				m_FSTAR[I] = m_FSTAR[I - 1];
			}

			m_FSTAR[0] = m_F;

			m_S = m_Sopt;
			m_F = m_Fopt;
			m_X = m_Xopt;
			m_AICC = m_AICCopt;
			
			

			//reset bounding parameter stat each loop
			for (size_t i = 0; i < m_XPstat.size(); i++)
			{
				m_XPstat[i].Reset();
			}
		}

		void Initialize(double T, long NEPS, double missingValue)
		{
			m_T = T;

			//  Initialize variable
			m_F = missingValue;
			m_FP = missingValue;
			m_Fopt = missingValue;
			m_AICC = m_AICCopt = m_AICCP = missingValue;
			m_MLL = m_MLLP = m_MLLopt = missingValue;

			m_NACC = 0;
			m_NOBDS = 0;
			m_NFCNEV = 0;

			//Initialize FSTAR
			m_FSTAR.clear();
			m_FSTAR.insert(m_FSTAR.begin(), NEPS, DBL_MAX);

		}

		std::vector<double> m_VM;
		CStatisticVector m_VMstat;


		std::vector<double> m_X;
		std::vector<double> m_XP;
		std::vector<double> m_Xopt;
		CStatisticVector m_XPstat;
		CStatisticVector m_Xstat;
		

		double m_T;

		std::vector<double> m_FSTAR;

		//output                
		double m_F;
		double m_FP;
		double m_Fopt;

		//output                
		double m_AICC;
		double m_AICCP;
		double m_AICCopt;

		double m_MLL;
		double m_MLLP;
		double m_MLLopt;

		//statistic
		CStatisticXY m_S;
		CStatisticXY m_SP;
		CStatisticXY m_Sopt;

		long m_NACC;
		long m_NFCNEV;
		long m_NOBDS;

		//constant 
		std::vector<double> m_C;
		CVariableBoundVector m_bounds;
	};

}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CSAControl& in, XmlElement& output)
	{
		XmlOut out(output);


		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::TYPE_OPTIMISATION)](in.m_bMax);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::STAT_OPTIMISATION)](in.m_statisticType);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::INITIAL_TEMPERATURE)](in.m_T);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::REDUCTION_FACTOR)](in.m_RT);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::EPSILON)](in.m_EPS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_CYCLE)](in.m_NS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION)](in.m_NT);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_EPSILON)](in.m_NEPS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_EVALUATION)](in.m_MAXEVL);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED1)](in.m_seed1);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED2)](in.m_seed2);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSAControl& out)
	{
		XmlIn in(input);

		std::string str;

		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::TYPE_OPTIMISATION)](out.m_bMax);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::STAT_OPTIMISATION)](out.m_statisticType);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::INITIAL_TEMPERATURE)](out.m_T);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::REDUCTION_FACTOR)](out.m_RT);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::EPSILON)](out.m_EPS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_CYCLE)](out.m_NS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION)](out.m_NT);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_EPSILON)](out.m_NEPS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_EVALUATION)](out.m_MAXEVL);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED1)](out.m_seed1);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED2)](out.m_seed2);


		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CVariableBound& in, XmlElement& output)
	{
		output.setValue(in.ToString());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CVariableBound& out)
	{
		std::string str;
		input.getValue(str);
		out.FromString(str);


		return true;
	}



	template <> inline
		void writeStruc(const WBSF::CSAParameter& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::NAME)](in.m_name);
		out[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::VALUE)](in.m_initialValue);
		out[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::BOUND)](in.m_bounds);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSAParameter& out)
	{
		XmlIn in(input);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::NAME)](out.m_name);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::VALUE)](out.m_initialValue);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::BOUND)](out.m_bounds);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CSAParameterVector& value, XmlElement& output)
	{
		std::for_each(value.begin(), value.end(),
			[&](const WBSF::CSAParameter & childVal)
		{
			XmlElement& newChild = output.addChild(WBSF::CSAParameter::GetXMLFlag());
			zen::writeStruc(childVal, newChild);
		}
		);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSAParameterVector& value)
	{
		bool success = true;
		value.clear();

		auto iterPair = input.getChildren(WBSF::CSAParameter::GetXMLFlag());
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			WBSF::CSAParameter childVal; //MSVC 2010 bug: cannot put this into a lambda body
			if (zen::readStruc(*iter, childVal))
				value.insert(value.end(), childVal);
			else
				success = false;
		}
		return success;
	}

}
