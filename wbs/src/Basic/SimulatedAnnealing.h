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
#include "Basic/UtilMath.h"



namespace WBSF
{

	//**********************************************************************
	//CSAControl
	class CSAControl
	{
	public:




		enum TMember { TYPE_OPTIMISATION, STAT_OPTIMISATION, INITIAL_TEMPERATURE, REDUCTION_FACTOR, REDUCTION_FACTOR2, EPSILON, P_EPSILON, NB_CYCLE, NB_ITERATION, NB_EPSILON, MAX_EVALUATION, SEED1, SEED2, NB_SKIP_LOOP, MAX_LOOP, NB_ITERATION2, NB_MEMBER };

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
			ASSERT(m_statisticType < NB_STATXY_TYPE);
			return AdjustFValue(stat[m_statisticType]);
		}

		double GetFinalFValue(const CStatisticXYEx& stat)
		{
			ASSERT(m_statisticType < NB_STATXY_TYPE_EX);
			return AdjustFValue(stat[m_statisticType]);
		}


		double GetVMiss() { return m_missing; }
		void SetVMiss(double in) { m_missing = in; }

		short m_statisticType;
		bool m_bMax;
		double m_RT;
		double m_RT2;
		double m_EPS;
		double m_P_EPS;
		long m_NS;
		long m_NT1;
		float m_NT2;//factor of iteration: NT = NT2* nb variables
		long m_NEPS;
		long m_MAXEVL;
		size_t m_max_loop;
		long m_seed1;
		long m_seed2;
		double m_T;
		long m_nbSkipLoop;

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


		bool IsOutOfBound(double& value, size_t digit = 5)const { return (WBSF::Round(value, digit) < WBSF::Round(m_lowerBound, digit)) || (value > WBSF::Round(m_upperBound, digit)); }
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
		double LimitTo(double v)const
		{
			return std::max(m_lowerBound, std::min(m_upperBound, v));
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
		enum TMember { NAME, VALUE, BOUND, USE_IT, NB_MEMBER };

		static const char* GetMemberName(int i) { _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag() { return XML_FLAG; }

		CSAParameter(std::string name = "", double value = 0, double lo = -1.0E25, double hi = 1.0E25);
		~CSAParameter();
		CSAParameter(const CSAParameter& in);

		void Reset();
		void clear() { Reset(); }
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
		bool m_b_use_it;

	protected:

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};



	typedef std::vector< CSAParameter > CSAParameterVector;
	typedef std::map<std::string, CSAParameterVector> CSAParametersMap;


	class CCComputationSet
	{
	public:

		CCComputationSet()
		{
			clear();
		}

		CCComputationSet(const CCComputationSet& in)
		{
			operator=(in);
		}

		CCComputationSet& operator=(const CCComputationSet& in)
		{
			m_X = in.m_X;
			m_Xstat = in.m_Xstat;
			m_F = in.m_F;
			m_AICC = in.m_AICC;
			m_n = in.m_n;
			m_k = in.m_k;
			m_MLL = in.m_MLL;
			m_S = in.m_S;

			return *this;
		}

		void clear()
		{
			m_X.clear();
			m_F = 0;
			m_Xstat.clear();
			m_AICC = 0;
			m_n = 0;
			m_k = 0;
			m_MLL = 0;
			m_S.clear();
		}

		void clear_stat()
		{
			for (size_t i = 0; i < m_Xstat.size(); i++)
				m_Xstat[i].clear();
		}

		void Initialize(double missingValue)
		{
			m_F = missingValue;
			m_n = 0;
			m_k = 0;
			m_AICC = missingValue;
			m_MLL = missingValue;
		}

		void resize(size_t size)
		{
			m_X.resize(size);
			m_Xstat.resize(size);
		}


		std::vector<double> m_X;
		CStatisticVector m_Xstat;
		double m_F;
		double m_AICC;
		size_t m_n;
		size_t m_k;
		double m_MLL;
		CStatisticXY m_S;

	};


	//*******************************************************************************
	//CComputationVariable
	class CComputationVariable : public CSAParameter
	{
	public:
		CComputationVariable() 
		{ 
			clear();
		}

		CComputationVariable(const CComputationVariable& in)
		{
			operator=(in);
		}

		const CComputationVariable& operator=(const CComputationVariable& in)
		{
			CSAParameter::operator=(in);
			//m_ = in.m_;
			//m_P = in.m_P;
			//m_opt = in.m_opt;
			//m_T = in.m_T;
			//m_FSTAR = in.m_FSTAR;
			//m_NFCNEV = in.m_NFCNEV;

			//m_VM = in.m_VM;
			//m_VMstat = in.m_VMstat;
			//
			//
			////constant 
			//m_C = in.m_C;
			//m_bounds = in.m_bounds;
			//m_b_use_it = in.m_b_use_it;

			m_VM=in.m_VM;
			m_C = in.m_C;
			m_VMstat = in.m_VMstat;

			return *this;
		}

		void clear()
		{
			CSAParameter::clear();
			//m_.clear();
			//m_P.clear();
			//m_opt.clear();
			//m_T=0;
			//m_NFCNEV = 0;
			//m_FSTAR.clear();

			//variables
			//m_VM.clear();
			//m_C.clear();
			//m_bounds.clear();
			//m_b_use_it.clear();
			m_VMstat.clear();
		}

		void clear_stat()
		{
			//CSAParameter::clear_stat();
			m_VMstat.clear();
		}

		//bool PrepareForAnotherLoop(const CSAControl& ctrl, size_t L)
		//{
		//	ASSERT(L > 0);//L in base 1

		//	//  Loop again.
		//	bool bQuit = fabs(m_.m_F - m_opt.m_F) <= ctrl.m_EPS;
		//	for (size_t i = 0; i < m_FSTAR.size() && bQuit; i++)
		//	{
		//		if (fabs(m_.m_F - m_FSTAR[i]) > ctrl.m_EPS)
		//			bQuit = false;
		//	}


		//	double F = ctrl.m_RT * pow(ctrl.m_RT2, L - 1);
		//	m_T = F * m_T;
		//	for (size_t I = m_FSTAR.size() - 1; I > 0; I--)
		//	{
		//		m_FSTAR[I] = m_FSTAR[I - 1];
		//	}

		//	m_FSTAR[0] = m_.m_F;

		//	m_ = m_opt;
		//	//(*this) = m_opt;

		//	//reset bounding parameter stat each loop
		//	for (size_t i = 0; i < m_P.m_Xstat.size(); i++)
		//		m_P.m_Xstat[i].clear();

		//	
		//	m_.m_Xstat = m_P.m_Xstat;

		//	//clean XPstat stats
		//	for (size_t i = 0; i < m_P.m_Xstat.size(); i++)
		//		m_P.m_Xstat[i].Reset();

		//	//clean VM stats
		//	for (size_t i = 0; i < m_VMstat.size(); i++)
		//		m_VMstat[i].Reset();

		//	

		//	return bQuit;
		//}



		//void Initialize(double T, long NEPS, double missingValue)
		//{

		//	m_T = T;

		//	//  Initialize variable
		//	//CCComputationSet::Initialize(missingValue);
		//	m_.Initialize(missingValue);
		//	m_P.Initialize(missingValue);
		//	m_opt.Initialize(missingValue);

		//	m_NFCNEV = 0;

		//	//Initialize FSTAR
		//	m_FSTAR.clear();
		//	m_FSTAR.insert(m_FSTAR.begin(), NEPS, DBL_MAX);

		//}

		

		

		//void resize(size_t size)
		//{
		//	//CCComputationSet::resize(size);
		//	//m_.resize(size);
		//	//m_P.resize(size);
		//	//m_opt.resize(size);
		//
		//
		//	m_VM.resize(size);
		//	m_VMstat.resize(size);
		//	m_C.resize(size);
		//	m_bounds.resize(size);
		//	m_b_use_it.resize(size);
		//}

		
		

		//variables
		//std::vector<double> m_VM;
		//std::vector<double> m_C;
		//CVariableBoundVector m_bounds;
		//std::vector<bool> m_b_use_it;
		//CStatisticVector m_VMstat;

		double m_VM;
		double m_C;
		//ndVector m_bounds;
		//bool> m_b_use_it;
		CStatistic m_VMstat;
		

	};

	class CComputationVariableVector : public std::vector<CComputationVariable>
	{
	public:

		void resize(size_t size)
		{
			vector<CComputationVariable>::resize(size);
			m_.resize(size);
			m_P.resize(size);
			m_opt.resize(size);
		}

		void clear_stat()
		{
			m_.clear_stat();
			m_P.clear_stat();
			m_opt.clear_stat();

			for (size_t i = 0; i < size(); i++)
				at(i).clear_stat();
		}

		ERMsg InitializeVar(const CSAParameterVector& parameters)
		{
			ERMsg msg;
			resize(parameters.size());
			for (size_t i = 0; i < parameters.size(); i++)
			{

				((CSAParameter&)at(i)) = parameters[i];
				//m_bounds[i] = parameters[i].m_bounds;
				m_P.m_X[i] = parameters[i].m_initialValue;
				at(i).m_C = 2;
				at(i).m_VM = at(i).m_bounds.GetExtent();
				//m_b_use_it[i] = parameters[i].m_b_use_it;

				//If the initial value is out of bounds, notify the user and return
				//to the calling routine.
				if (at(i).m_bounds.IsOutOfBound(m_P.m_X[i]))
					msg.ajoute("The starting value (" + ToString(m_P.m_X[i]) + ") is not inside the bounds [" + ToString(at(i).m_bounds.GetLowerBound()) + "," + ToString(at(i).m_bounds.GetUpperBound()) + "].");
			}

			return msg;
		}

		bool PrepareForAnotherLoop(const CSAControl& ctrl, size_t L)
		{
			ASSERT(L > 0);//L in base 1

			//  Loop again.
			bool bQuit = fabs(m_.m_F - m_opt.m_F) <= ctrl.m_EPS;
			for (size_t i = 0; i < m_FSTAR.size() && bQuit; i++)
			{
				if (fabs(m_.m_F - m_FSTAR[i]) > ctrl.m_EPS)
					bQuit = false;
			}


			double F = ctrl.m_RT * pow(ctrl.m_RT2, L - 1);
			m_T = F * m_T;

			for (size_t i = 0; i < m_FSTAR.size()-1; i++)
			{
				m_FSTAR[i+1] = m_FSTAR[i];
			}

			m_FSTAR[0] = m_.m_F;

			m_ = m_opt;
			
			//reset bounding parameter stat each loop
			clear_stat();

			return bQuit;
		}



		void Initialize(double T, long NEPS, double missingValue)
		{

			m_T = T;

			//  Initialize variable
			//CCComputationSet::Initialize(missingValue);
			m_.Initialize(missingValue);
			m_P.Initialize(missingValue);
			m_opt.Initialize(missingValue);

			m_NFCNEV = 0;

			//Initialize FSTAR
			m_FSTAR.clear();
			m_FSTAR.insert(m_FSTAR.begin(), NEPS, DBL_MAX);

		}

		CCComputationSet m_;
		CCComputationSet m_P;
		CCComputationSet m_opt;

		double m_T;
		long m_NFCNEV;
		std::vector<double> m_FSTAR;
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
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::REDUCTION_FACTOR2)](in.m_RT2);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::EPSILON)](in.m_EPS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::P_EPSILON)](in.m_P_EPS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_CYCLE)](in.m_NS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION)](in.m_NT1);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION2)](in.m_NT2);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_EPSILON)](in.m_NEPS);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_EVALUATION)](in.m_MAXEVL);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_LOOP)](in.m_max_loop);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED1)](in.m_seed1);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED2)](in.m_seed2);
		out[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_SKIP_LOOP)](in.m_nbSkipLoop);

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
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::REDUCTION_FACTOR2)](out.m_RT2);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::EPSILON)](out.m_EPS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::P_EPSILON)](out.m_P_EPS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_CYCLE)](out.m_NS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION)](out.m_NT1);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_ITERATION2)](out.m_NT2);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_EPSILON)](out.m_NEPS);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_EVALUATION)](out.m_MAXEVL);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::MAX_LOOP)](out.m_max_loop);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED1)](out.m_seed1);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::SEED2)](out.m_seed2);
		in[WBSF::CSAControl::GetMemberName(WBSF::CSAControl::NB_SKIP_LOOP)](out.m_nbSkipLoop);


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
		out[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::USE_IT)](in.m_b_use_it);


	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSAParameter& out)
	{
		XmlIn in(input);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::NAME)](out.m_name);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::VALUE)](out.m_initialValue);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::BOUND)](out.m_bounds);
		in[WBSF::CSAParameter::GetMemberName(WBSF::CSAParameter::USE_IT)](out.m_b_use_it);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CSAParameterVector& value, XmlElement& output)
	{
		std::for_each(value.begin(), value.end(),
			[&](const WBSF::CSAParameter& childVal)
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
