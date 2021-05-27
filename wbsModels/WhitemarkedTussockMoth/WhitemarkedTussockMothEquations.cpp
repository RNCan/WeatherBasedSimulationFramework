//*****************************************************************************
// File: WhitemarkedTussockMothEquations.h
//
// Class: CWhitemarkedTussockMothEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 22/04/2019   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "WhitemarkedTussockMothEquations.h"
#include "ModelBase/DevRateEquation.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace WBSF;
using namespace WTM;
using namespace std;

namespace WBSF
{
//phi0 = 0.68910
//bb0 = 0.00250
//Tb0 = 13.20050 {  
//Tm0 = 47.59800 {  
//deltab0 = 3.73447 
//deltam0 = 0.57302 
//phi1 = 0.04000 {  
//bb1 = 0.03480 {   
//Tb1 = 0.98756 {   
//Tm1 = 25.20095 {  
//deltab1 = 1.78538 
//deltam1 = 3.82664 
//phi2 = 0.59070 {  
//bb2 = 0.12880 {   
//Tb2 = 7.68840 {  
//Tm2 = 22.79988 {  
//deltab2 = 8.82424 
//deltam2 = 6.23350 
//phi3 = 0.06130 {  
//bb3 = 0.11130 {   
//Tb3 = -3.23070 {  
//Tm3 = 18.84868 {  
//deltab3 = 5.78509 
//deltam3 = 0.87032 
//a0 = 5.18580 {  
//b0 = 46.93140 { 
//a1 = 0.04480 {  
//b1 = 10.02700 { 
//a2 = 1.59010 {  
//b2 = 8.38944 { 
//a3 = 2.38173 {  
//b3 = 43.14400 {   
//start = 120.00000 
//EggFactor1 = 0.250


//N = 32488	T = 0.62500	F = 6631.94902
//NbVal = 42	Bias = -3.60585	MAE = 10.14771	RMSE = 12.56597	CD = 0.93686	R² = 0.94231
//phi0 = 0.77500 { 
//bb0 = 0.01923 {  
//Tb0 = 8.33336 {  
//Tm0 = 32.80238 { 
//deltab0 = 6.74052
//deltam0 = 9.72905
//phi1 = 0.04365 { 
//phi3 = 0.04258 { 
//bb3 = 0.02753 {  
//Tb3 = 10.70819 { 
//Tm3 = 34.17816 { 
//deltab3 = 0.64844
//deltam3 = 0.10197
//a0 = 7.94386 {   
//b0 = 36.66289 {  
//a1 = 0.78522 {   
//b1 = 49.37872 {  
//a3 = 2.99980 {   
//b3 = 46.47272 {  
//mu = 408.60058 { 
//ss = 0.86130 {   
//DDThreshold = 3.00062
//EggFactor1 = 0.58918 

	//phi,bb,Tb,Tm,deltab,deltam
	//{0.044, 0.02, 12.2, 34.2, 5.44, 1.17}, 
	const double CWhitemarkedTussockMothEquations::P[NB_STAGES][NB_DEV_RATE_PARAMS] =
	{

		{0.77500, 0.01923, 8.33336, 32.8023, 6.74052, 9.72905},
		{0.044, 0.02, 12.2, 34.2, 5.44, 1.17},
		{0,0,0,0,0,0},//Pupae
		{0.04258, 0.02753,10.70819, 34.17816, 0.64844, 0.10197},

		//{0.68910,0.00250,13.20050,47.59800,3.73447,0.57302 },
		//{0.04000,0.03480 ,0.98756,25.2009,1.78538 ,3.82664 },
		//{0.5907,0.12880,7.68840,22.7998,8.82424 ,6.23350 },
		//{0.0613,0.11130,-3.2307,28.8486,5.78509 ,0.87032 }
	};

	//cc=3.073075e-03 k=2.754737e+04 Tmin=1.224135e+01 Tmax=3.157231e+01
	//const double CWhitemarkedTussockMothEquations::P[NB_STAGES][NB_DEV_RATE_PARAMS] =
	//{
	//	{0.003073, 27547, 12.2, 31.6, 0, 0},
	//	{0.003073, 27547, 12.2, 31.6, 0, 0},
	//	{0.003073, 27547, 12.2, 31.6, 0, 0},
	//	{0.003073, 27547, 12.2, 31.6, 0, 0},
	//};

	const double CWhitemarkedTussockMothEquations::D[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		//{ 0.016, 49.9 },//Egg
		//{ 0.034, 6.55 },//Larvae
		//{ 8.730, 5.24 },//Pupae
		//{ 7.422, 19.3 },//Adult

		//{5.18580,46.9314},
		//{0.04480,10.0270},
		//{1.59010,8.38944},
		//{2.38173,43.1440},
	
		{7.9439, 36.6629},
		{0.7852, 49.3787},
		{0.0000, 00.0000},//Pupae
		{2.9998, 46.4727}

	};

	
	const double CWhitemarkedTussockMothEquations::H[NB_HATCH_PARAMS] = { 60, 408.6, 0.861, 3.0 };

	CWhitemarkedTussockMothEquations::CWhitemarkedTussockMothEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_DEV_RATE_PARAMS; p++)
			{
				m_P[s][p] = P[s][p];
			}

			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_D[s][p] = D[s][p];
			}
		}


		for (size_t p = 0; p < NB_HATCH_PARAMS; p++)
			m_H[p] = H[p];

	}


	//Daily development rate
	double CWhitemarkedTussockMothEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		if (s == PUPAE)
			return 1;


		double r = 0;

		//if (s == ADULT)
		//{
		//	//fixed longevity for simplification: no scientific data
		//	r = 1.0 / m_H[μ];
		//}
		//else
		//{
		const vector <double> p(m_P[s], m_P[s] + sizeof m_P[s] / sizeof m_P[s][0]);
		r = max(0.0, min(1.0, CDevRateEquation::GetRate(CDevRateEquation::Regniere_2012, p, T)));
		//}


//		r = max(0.0, CDevRateEquation::GetFValue(CDevRateEquation::HueyStevenson_1979, p, T));
		
		//attention il y a un moins ici. faut changer cela plus tard
		/*r = max(0.0, -m_P[s][0] + m_P[s][1] * T);
		if (s == LARVAE)
			r = max(0.0, -0.037 + 0.003 * T);*/

		
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}


	double CWhitemarkedTussockMothEquations::GetHatchCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_H[μ], m_H[ѕ]);

		double DD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		//while (DD < 0 || DD>2000)
			DD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return DD;
	}


	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CWhitemarkedTussockMothEquations::GetRelativeDevRate(size_t s)const
	{
		double rr = 0;

		if (s == PUPAE)
			return 1;
		
		double Э = m_randomGenerator.Randu(true, true);
		
		/*if(s==EGG)
			rr = m_D[s][к] * pow(-log(1.0 - Э), 1.0 / m_D[s][Ϙ]);
		else*/
			rr = 1.0 - log((pow(Э, -m_D[s][Ϙ]) - 1.0) / (pow(0.5, -m_D[s][Ϙ]) - 1.0)) / m_D[s][к];//add -q by RSA 
			

		
		rr = max(0.01, min(100.0, rr));
		
		//no limitation during the calibration phase, to add
		/*while (rr<0.4 || rr>2.5)
		{
			double Э = m_randomGenerator.Randu(true, true);
			rr = 1 - log((pow(Э, -m_D[s][Ϙ]) - 1) / (pow(0.5, -m_D[s][Ϙ]) - 1)) / m_D[s][к];
		}*/

		while (rr<0.2 || rr>5)
		{
			double Э = m_randomGenerator.Randu(true, true);
			rr = 1 - log((pow(Э, -m_D[s][Ϙ]) - 1) / (pow(0.5, -m_D[s][Ϙ]) - 1)) / m_D[s][к];
		}

		_ASSERTE(!_isnan(rr) && _finite(rr));
		return rr;
	}



	//*****************************************************************************
	//

	//l: longevity [days]
	double CWhitemarkedTussockMothEquations::GetFecondity()const
	{
		//(Wagner 2005).
		//100-300 eggs

		double f = m_randomGenerator.Rand(100, 300);
		return f;
	}



}

