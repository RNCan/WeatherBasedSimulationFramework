
#include <vector>
#include <bitset>
#include "Basic/SimulatedAnnealing.h"


namespace WBSF
{


	//**************************************************************************************************
	//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads
	class CDevRateEquation
	{
	public:

		enum { EQ_NAME, EQ_R, EQ_PARAM, NB_INFO };
		enum TParameters { P0, P1, P2, P3, P4, P5, P6, P7, NB_P_MAX };
		enum TDevRateEquation
		{
			Unknown=-1, FIRST_EQUATIONS=0,
			Allahyari= FIRST_EQUATIONS, Analytis_1977, Angilletta_2006, Bieri_1983, Briere1_1999, Briere2_1999, Damos_2008, Damos_2011,
			DevaHiggis, Exponential, HilbertLogan_1983, HilbertLoganIII, HueyStevenson_1979,
			Janisch1_1932, Janisch2, Kontodimas_2004, Lactin1_1995, Lactin2_1995,
			Lamb_1992, Logan6_1976, Logan10_1976, LoganExponential, LoganTb,
			Poly1, Poly2, Poly3, Poly4, Pradham, RatkowskySquare, Ratkowsky_1983,
			Regniere_1982, Regniere_1987, Regniere_2012, SchoolfieldHigh_1981, SchoolfieldLow_1981,
			Schoolfield_1981, SharpeDeMichele3, SharpeDeMichele_1977,
			Shi_2011, Shi_beta_2016, SaintAmant_2019, Stinner_1974, 
			Taylor_1981, Wagner_1988, Wang_1982, Wangengel_1998, Yin_beta_1995,
			NB_EQUATIONS
		};
		


		static TDevRateEquation eq(size_t e) { _ASSERTE(e < NB_EQUATIONS); return (TDevRateEquation)e; }
		static double GetRate(TDevRateEquation model, const std::vector<double>& P, double T);
		static const char* GetEquationName(TDevRateEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_NAME]; }
		static const char* GetEquationR(TDevRateEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_R]; }


		static CSAParameterVector GetParameters(TDevRateEquation model);
		static CSAParameterVector GetParameters(TDevRateEquation model, const std::vector<double>& P);
		static bool IsParamValid(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P);


	protected:

		static const char* EQUATION[NB_EQUATIONS][NB_INFO];
	};

	class CDevRateEqSelected : public std::bitset<CDevRateEquation::NB_EQUATIONS>
	{
	public:

		using std::bitset<CDevRateEquation::NB_EQUATIONS>::operator=;

		bool operator==(const CDevRateEqSelected& in)const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::operator==(in); }
		bool operator!=(const CDevRateEqSelected& in)const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::operator!=(in); }

		std::string GetSelection()const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::to_string(); }
		void SetSelection(const std::string& in) { std::bitset<CDevRateEquation::NB_EQUATIONS>::operator = (std::bitset<CDevRateEquation::NB_EQUATIONS>(in)); }

	};

	typedef CDevRateEquation::TDevRateEquation TDevRateEquation;
}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CDevRateEqSelected& in, XmlElement& output)
	{
		output.setValue(in.GetSelection());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CDevRateEqSelected& out)
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