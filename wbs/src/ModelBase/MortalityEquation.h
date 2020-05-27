
#include <vector>
#include <bitset>
#include "Basic/SimulatedAnnealing.h"


namespace WBSF
{


	//**************************************************************************************************
	//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads
	class CMortalityEquation
	{
	public:

		enum { EQ_NAME, EQ_R, EQ_PARAM, NB_INFO };
		enum TParameters { P0, P1, P2, P3, P4, P5, P6, P7, NB_P_MAX };
		enum TMortalityEquation
		{
			Unknown=-1, M01=0,
			M45 = 44,
			NB_EQUATIONS
		};

		static TMortalityEquation eq(size_t e) { _ASSERTE(e < NB_EQUATIONS); return (TMortalityEquation)e; }
		static double GetMortality(TMortalityEquation model, const std::vector<double>& P, double T);
		static const char* GetEquationName(TMortalityEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_NAME]; }
		static const char* GetEquationR(TMortalityEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_R]; }


		static CSAParameterVector GetParameters(TMortalityEquation model);
		static CSAParameterVector GetParameters(TMortalityEquation model, const std::vector<double>& P);
		static bool IsParamValid(CMortalityEquation::TMortalityEquation model, const std::vector<double>& P);


	protected:

		static const char* EQUATION[NB_EQUATIONS][NB_INFO];
	};

	class CMortalityEqSelected : public std::bitset<CMortalityEquation::NB_EQUATIONS>
	{
	public:

		using std::bitset<CMortalityEquation::NB_EQUATIONS>::operator=;

		bool operator==(const CMortalityEqSelected& in)const { return std::bitset<CMortalityEquation::NB_EQUATIONS>::operator==(in); }
		bool operator!=(const CMortalityEqSelected& in)const { return std::bitset<CMortalityEquation::NB_EQUATIONS>::operator!=(in); }

		std::string GetSelection()const { return std::bitset<CMortalityEquation::NB_EQUATIONS>::to_string(); }
		void SetSelection(const std::string& in) { std::bitset<CMortalityEquation::NB_EQUATIONS>::operator = (std::bitset<CMortalityEquation::NB_EQUATIONS>(in)); }

	};

	typedef CMortalityEquation::TMortalityEquation TMortalityEquation;
}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CMortalityEqSelected& in, XmlElement& output)
	{
		output.setValue(in.GetSelection());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CMortalityEqSelected& out)
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