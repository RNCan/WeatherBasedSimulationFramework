
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

		enum { EQ_NAME, EQ_R, EQ_PARAM, NB_INFO};
		enum TParameters { P0, P1, P2, P3, P4, P5, P6, P7 };
		enum TDevRateEquation
		{
			//DevaHiggis, Logan1_1976, Logan2_1976, Briere1, Briere2, HilberLogan1, HilberLogan_1983, Latin2,
			//Linear, Poly2, Poly3, Poly4, ExponentialSimple, TbLogan, ExponentialLogan, 
			//Ratkowsky1, Ratkowsky2, Davidson_1944, Pradham,  
			//Allahyari, Kontodimas_2004,Janisch1, Janisch2, Stinner1, Stinner2, Stinner3, Stinner4, 
			//Taylor_1981, Lactin1_1995, Lactin2_1995, Logistic, Regniere1_1987,
			//Analytis_1977, /*Bayoh_2003,*/ Shi_2016,Yin_1995,Bieri_1983, Damos_2008,Damos_2011,
			//Lamb_1992,Shi_Perf2_2011,Rootsq_1982,Schoolfield_1981,SchoolfieldHigh_1981,SchoolfieldLow_1981,
			//Shi_2011,Wagner_1988,Wang_1982,Wangengel_1998,

			Allahyari, Analytis_1977, Bieri_1983, Briere1_1999, Briere2_1999, Damos_2008, Damos_2011,
			Davidson_1944, DevaHiggis, Exponential, HilberLogan_1983, HilberLoganIII,
			Janisch1_1932, Janisch2, Kontodimas_2004, Lactin1_1995, Lactin2_1995,
			Lamb_1992, Logan6_1976, Logan10_1976, LoganExponential, LoganTb,
			Poly1, Poly2, Poly3, Poly4, Pradham, RatkowskySquare, Ratkowsky_1983,
			Regniere_1987, Regniere_2012, SchoolfieldHigh_1981, SchoolfieldLow_1981,
			Schoolfield_1981, SharpeDeMichele3, SharpeDeMichele4, SharpeDeMichele5, SharpeDeMichele6,
			SharpeDeMichele10, SharpeDeMichele11, SharpeDeMichele13, SharpeDeMichele14, SharpeDeMichele_1977,
			Shi_2011, Shi_Perf2_2011, Shi_beta_2016, Stinner1, Stinner2, Stinner_1974,
			Stinner4, Taylor_1981, Wagner_1988, Wang_1982, Wangengel_1998, Yin_beta_1995,
			NB_EQUATIONS
		};

		static TDevRateEquation e(size_t e) { _ASSERTE(e < NB_EQUATIONS); return (TDevRateEquation)e; }
		static double GetFValue(TDevRateEquation model, const std::vector<double>& P, double T);
		static const char* GetEquationName(TDevRateEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_NAME]; }
		static const char* GetEquationR(TDevRateEquation model) { _ASSERTE(model < NB_EQUATIONS); return EQUATION[model][EQ_R]; }
		
		static CSAParameterVector GetParameters(TDevRateEquation model);
//		static CSAParameterVector GetParameters_old(TDevRateEquation model);
		static CSAParameterVector GetParameters(TDevRateEquation model, const std::vector<double>& X);

	protected:

		static const char* EQUATION[NB_EQUATIONS][NB_INFO];
	};

	class EquationBitset : public std::bitset<CDevRateEquation::NB_EQUATIONS>
	{
	public:

		using std::bitset<CDevRateEquation::NB_EQUATIONS>::operator=;

		bool operator==(const EquationBitset& in)const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::operator==(in); }
		bool operator!=(const EquationBitset& in)const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::operator!=(in); }

		std::string GetSelection()const { return std::bitset<CDevRateEquation::NB_EQUATIONS>::to_string(); }
		void SetSelection(const std::string& in) { std::bitset<CDevRateEquation::NB_EQUATIONS>::operator = (std::bitset<CDevRateEquation::NB_EQUATIONS>(in)); }

	};
}