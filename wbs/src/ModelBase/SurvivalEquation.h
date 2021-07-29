
#include <vector>
#include <bitset>
#include "Basic/SimulatedAnnealing.h"


namespace WBSF
{
	//**************************************************************************************************
	//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads
	class CSurvivalEquation
	{
	public:

		enum { EQ_NAME, EQ_R, EQ_PARAM, R_MATH_PLOT, NB_INFO };
		enum TParameters { P0, P1, P2, P3, P4, P5, P6, P7, NB_P_MAX };
		enum TSurvivalEquation
		{
			Unknown=-1, 
			Survival_01, Survival_02, Survival_03, Survival_04, Survival_05, Survival_06,
			Survival_07, Survival_08, Survival_09, Survival_10, Survival_11,
			Survival_12, Survival_13, Survival_14, Survival_15/*GompertzMakeham*/, Survival_16/*wang2*/,
			NB_EQUATIONS
		};

		static TSurvivalEquation eq(size_t e) { _ASSERTE(e < NB_EQUATIONS); return (TSurvivalEquation)e; }
		static double GetSurvival(TSurvivalEquation model, const std::vector<double>& P, double T);
		static const char* GetEquationName(TSurvivalEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_NAME]; }
		static const char* GetEquationR(TSurvivalEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_R]; }
		static const char* GetMathPlot(TSurvivalEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][R_MATH_PLOT]; }


		static CSAParameterVector GetParameters(TSurvivalEquation model);
		static CSAParameterVector GetParameters(TSurvivalEquation model, const std::vector<double>& P);
		static bool IsParamValid(CSurvivalEquation::TSurvivalEquation model, const std::vector<double>& P);

	protected:

		static const char* EQUATION[NB_EQUATIONS][NB_INFO];
	};

	class CSurvivalEqSelected : public std::bitset<CSurvivalEquation::NB_EQUATIONS>
	{
	public:

		using std::bitset<CSurvivalEquation::NB_EQUATIONS>::operator=;

		bool operator==(const CSurvivalEqSelected& in)const { return std::bitset<CSurvivalEquation::NB_EQUATIONS>::operator==(in); }
		bool operator!=(const CSurvivalEqSelected& in)const { return std::bitset<CSurvivalEquation::NB_EQUATIONS>::operator!=(in); }

		std::string GetSelection()const { return std::bitset<CSurvivalEquation::NB_EQUATIONS>::to_string(); }
		void SetSelection(const std::string& in) { std::bitset<CSurvivalEquation::NB_EQUATIONS>::operator = (std::bitset<CSurvivalEquation::NB_EQUATIONS>(in)); }

	};

	typedef CSurvivalEquation::TSurvivalEquation TSurvivalEquation;
}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CSurvivalEqSelected& in, XmlElement& output)
	{
		output.setValue(in.GetSelection());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSurvivalEqSelected& out)
	{
		std::string str;
		try
		{
			if (input.getValue(str))
				out.SetSelection(str);
		}
		catch (...)
		{
		}

		return true;
	}
}