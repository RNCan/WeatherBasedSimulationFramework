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
//Eps = [0]{ 0.00000,  0.00000,  0.00000,  0.00000}
//***********************************
//13 mars 2019 15:43:33

	const double CLaricobiusNigrinusEquations::P[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		{ -33.52443, 79.04205 },//Egg
		{ -4.42992,  40.63253 },//L1
		{ -4.42992,  40.63253 },//L2
		{ -4.42992,  40.63253 },//L3
		{ -4.42992,  40.63253 },//L4
		{ -1.80, 11.60 },//PrePupae
		{ -1.80, 11.60 },//Pupae
		{ -1.80, 11.60 },//Adult
	};

	const double CLaricobiusNigrinusEquations::F[4] = { 0.25,0.25,0.25,0.25 };


	CLaricobiusNigrinusEquations::CLaricobiusNigrinusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 30, 0.25)
	{

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_P[s][p] = P[s][p];
			}
		}

		for (size_t p = 0; p < 4; p++)
			m_F[p] = F[p];
		
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

		

//		12 mars 2019 01:12:47
//Egg: Eq7
//N=     97201	T=  0.00019	F= 0.00138
//NbVal=     6	Bias=-0.00223	MAE= 0.01199	RMSE= 0.01519	CD= 0.97093	R²= 0.97217
//0.87742,0.93156  ,0.86978  ,0.14425  ,18.13652  ,22.18765  
         

//Larva: Eq7
//N=     93601	T=  0.00031	F= 0.00006
//NbVal=     6	Bias= 0.00015	MAE= 0.00265	RMSE= 0.00313	CD= 0.98959	R²= 0.98961
//0.64782,0.39320,1.11128,0.18811,0.71756,39.52194  

//Prepupa: Eq7
//N=     94801	T=  0.00027	F= 0.00000
//NbVal=     5	Bias= 0.00012	MAE= 0.00087	RMSE= 0.00100	CD= 0.99749	R²= 0.99753
//0.59803,0.31393,1.45562,0.59397,27.59838,38.32248  

//Pupa: Eq7
//N=     88801	T=  0.00060	F= 0.00003
//NbVal=     5	Bias= 0.00016	MAE= 0.00187	RMSE= 0.00255	CD= 0.98291	R²= 0.98312
//0.30749,0.50343,9.32570,1.59475,37.95035,3.39897  


		double r = 0;

		//Equation from
		//Temperature - Dependent Development of the Specialist Predator Laricobius nigrinus(Coleoptera: Derodontidae)
		//G.M.G.ZILAHI-BALOGH, S.M.SALOM, AND L.T.KOK (2003) Entomological Society of America
		switch (s)
		{
		case EGG:	r = max(0.0, -0.0907 + 0.0165*T); break;
		case L1:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[0]); break;
		case L2:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[1]); break;
		case L3:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[2]); break;
		case L4:	r = max(0.0, (-0.0151 + 0.0048*T)/m_F[3]); break;
		case PREPUPAE:	r = max(0.0, -0.0132 + 0.0047* min(6.0, T) ); break;//in the ground: need ground temperature
		case PUPAE:	r = max(0.0, -0.0144 + 0.0047* min(6.0, T)); break;//in the ground: need ground temperature
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

	//Fecondity
	//100.8 ± 89.6 (range, 2 - 396) eggs.

}