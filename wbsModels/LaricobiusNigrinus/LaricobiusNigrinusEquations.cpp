//*****************************************************************************
// File: LaricobiusNigrinusEquations.h
//
// Class: CLaricobiusNigrinusEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 10/03/2019   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LaricobiusNigrinusEquations.h"


using namespace WBSF;
using namespace LNF;
using namespace std;

namespace WBSF
{


//N=     54001	T=  0.00508	F=5171.58457
//NbVal=   107	Bias=-0.53334	MAE= 3.96183	RMSE= 6.95216	CD= 0.97125	R²= 0.97258
//a1                  	= -33.52443  b1                  	=  79.04205  a2                  	=  -4.42992  b2                  	=  40.63253  peak                	=  51.06799  s                   	=  17.23233  
//***********************************
//13 mars 2019 15:43:33
//N=     42501	T=  0.00846	F=3221.52351
//NbVal=   117	Bias=-0.26109	MAE= 3.38268	RMSE= 5.24732	CD= 0.98206	R²= 0.98262
//f1                  	=  -0.45249  f2                  	=   3.17877  f3                  	=  -0.51340  f4                  	=   2.14575  maxTsoil            	=   4.80572  
//***********************************
//14 mars 2019 06:32:07
         
	const double CLaricobiusNigrinusEquations::P[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		{ -33.52, 79.04 },//Egg
		{ -4.43,  40.63 },//Larvae
		{ -0.452, 3.179 },//PrePupae
		{ -0.513, 2.146 },//Pupae
		{  0.000, 0.000 },//Adult
	};

	

	CLaricobiusNigrinusEquations::CLaricobiusNigrinusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, -10, 30, 0.25)
	{

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_P[s][p] = P[s][p];
			}
		}
	}



	//double CLaricobiusNigrinusEquations::Equation1(size_t e, double T)
	//{
	//	const double* p = P[e];//current P for equation
	//	size_t s = e2s(e);//compute stage for b1Factor

	//	double Tau = (T - p[PTB]) / (p[PTM] - p[PTB]);
	//	double p1 = 1 / (1 + exp(p[PB2] - p[PB3] * Tau));
	//	double p2 = exp((Tau - 1) / p[PB4]);
	//	double Rt = p[PB1] * b1Factor[s] * (p1 - p2);

	//	return max(0.0, Rt);
	//}

	//double CLaricobiusNigrinusEquations::Equation2(size_t e, double T)
	//{
	//	const double* p = P[e];//current P for equation
	//	size_t s = e2s(e);//compute stage for b1Factor

	//	double p1 = exp(-0.5*Square((T - p[PB2]) / p[PB3]));
	//	double Rt = p[PB1] * b1Factor[s] * p1;

	//	return max(0.0, Rt);
	//}

	//double CLaricobiusNigrinusEquations::Equation3(size_t e, double T)
	//{
	//	const double* p = P[e];//current P for equation
	//	size_t s = e2s(e);//compute stage for b1Factor
	//	ASSERT(s == ADULT);
	//	
	//	T = max(8.0, min(35.0, T) );
	//	double Rt = 1 / (p[PB1] * b1Factor[s] + p[PB2] * T + p[PB3] * T*T);
	//	return max(0.0, Rt);
	//}


	//Egg hatch of forest tent caterpillar (Lepidoptera:Lasiocampidae) on two preferred host species
	//David R.Gray, 1 Don P.Ostaff
	//Can. Entomol. 144: 790–797 (2012)



	//Daily development rate
	double CLaricobiusNigrinusEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);
	
		double r = 0;

		//double Tsoil = min(5.3, T);// 0.7*T + 1.54;
		//Equation from
		//Temperature - Dependent Development of the Specialist Predator Laricobius nigrinus(Coleoptera: Derodontidae)
		//G.M.G.Zilahi-Balogh, S.M.SALOM, AND L.T.KOK (2003) Entomological Society of America
		switch (s)
		{
		case EGG:	r = max(0.0, -0.0907 + 0.0165*T); break;
		//case L1:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[0]); break;
		//case L2:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[1]); break;
		//case L3:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[2]); break;
		//case L4:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[3]); break;
		case LARVAE: r = max(0.0, -0.0151 + 0.0048*T ); break;
		case PREPUPAE:	r = max(0.0, -0.0132 + 0.0047* T); break;//in the ground: need ground temperature
		case PUPAE:	r = max(0.0, -0.0144 + 0.0047* T); break;//in the ground: need ground temperature
		case ADULT:	r = 1.0 / 172.0; break;//from October to April
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 
	

	double CLaricobiusNigrinusEquations::GetRelativeDevRate(size_t s)const
	{
		double rr = 0;

		double Э = m_randomGenerator.Randu(true, true);
		rr = 1.0 - log((pow(Э, m_P[s][Ϙ]) - 1.0) / (pow(0.5, m_P[s][Ϙ]) - 1.0)) / m_P[s][к];
		while (rr<0.4 || rr>2.5)
		{
			double Э = m_randomGenerator.Randu(true, true);
			rr = 1 - log((pow(Э, m_P[s][Ϙ]) - 1) / (pow(0.5, m_P[s][Ϙ]) - 1)) / m_P[s][к];
		}

		_ASSERTE(!_isnan(rr) && _finite(rr));
		return rr;
	}

	//*****************************************************************************
	//

	//l: longevity [days]
	double CLaricobiusNigrinusEquations::GetFecondity(double l)const
	{
		//Fecondity from Zilahi-Balogh(2001)
		//100.8 ± 89.6 (range, 2 - 396) eggs.

		double w = l / 7.0;//[days] --> [weeks]
		double fm = 6.1*w + 20.1;
		double fs = 2.6*w + 13.3;

		double f = m_randomGenerator.RandNormal(fm, fs);

		while(f<2||f>396)
			m_randomGenerator.RandNormal(fm, fs);


		return f; 
	}

	double CLaricobiusNigrinusEquations::GetAdultLongevity()const
	{
		//Longevity of female
		//value from fecundity/longevity Zilahi-Balogh(2001)

		static const double P[2] = { 95.07427, 58.04989 };

		double l = m_randomGenerator.RandNormal(P[0], P[1]);
		while(l<14||l>224)
			l = m_randomGenerator.RandNormal(P[0], P[1]);
		
		return l;
	}
}