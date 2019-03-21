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
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace WBSF;
using namespace LNF;
using namespace std;

namespace WBSF
{ 
//NbVal=   201	Bias= 1.35004	MAE= 6.47098	RMSE=10.14646	CD= 0.90988	R²= 0.91441
//a1                  	=  7.79345 
//b1                  	=  36.52359 
//a2                  	=  0.01016 
//b2                  	=  13.13306 
//mu                  	= 101.39193 
//s                   	=  27.21807 
//DDThreshold         	=   4.27488 
                
              

	const double CLaricobiusNigrinusEquations::D[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		{ 7.79, 36.5 },//Egg
		{ 0.01, 13.1 },//Larvae
		{ 0.00 , 0.00 },//PrePupae
		{ 0.00 , 0.00 },//Pupae
	};

	const double CLaricobiusNigrinusEquations::O[NB_OVIP_PARAMS] = { 101.4, 27.2, 4.3 };
	

	CLaricobiusNigrinusEquations::CLaricobiusNigrinusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, -10, 30, 0.25)
	{

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_D[s][p] = D[s][p];
			}
		}

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			m_O[p] = O[p];
	}

	

	double CLaricobiusNigrinusEquations::Eq7(size_t s, double T)
	{
		static const double p[2][6] = 
		{
			{ 0.87742, 0.93156, 0.86978, 0.14425, 18.13652, 22.18765 },
			{0.64782, 0.3932, 1.11128, 0.18811, 30.71756, 39.52194}
		};

		double Tau = (T - p[s][4]) / (p[s][5] - p[s][4]);
		double p1 = 1 / (1 + exp(p[s][1] - p[s][2] * Tau));
		double p2 = exp((Tau - 1) / p[s][3]);
		double Rt = p[s][0] * (p1 - p2);

		return max(0.0, Rt);
	}


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
		case EGG:	r = Eq7(s, T); break;
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
		double rr = 1;

		if (s == EGG || s == LARVAE)
		{
			double Э = m_randomGenerator.Randu(true, true);
			rr = 1.0 - log((pow(Э, -m_D[s][Ϙ]) - 1.0) / (pow(0.5, -m_D[s][Ϙ]) - 1.0)) / m_D[s][к];//add -q by RSA 
			while (rr<0.4 || rr>2.5)
			{
				double Э = m_randomGenerator.Randu(true, true);
				rr = 1 - log((pow(Э, -m_D[s][Ϙ]) - 1) / (pow(0.5, -m_D[s][Ϙ]) - 1)) / m_D[s][к];
			}
		}

		_ASSERTE(!_isnan(rr) && _finite(rr));
		return rr;
	}

	double CLaricobiusNigrinusEquations::GetOvipositionDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_O[μ], m_O[ѕ]);

		double DD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (DD < 0 || DD>5000)
			DD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return DD;
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
			f = m_randomGenerator.RandNormal(fm, fs);


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

