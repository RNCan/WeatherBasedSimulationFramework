//*****************************************************************************
// File: ForestTentCaterpillarEquations.h
//
// Class: CForestTentCaterpillarEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 05/03/2019   Rémi Saint-Amant    Creation from new paper
//*****************************************************************************
#include "ForestTentCaterpillarEquations.h"


using namespace WBSF;
using namespace FTC;
using namespace std;

namespace WBSF
{
	CForestTentCaterpillarEquations::CForestTentCaterpillarEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 30, 0.25)
	{
	}



	//double CForestTentCaterpillarEquations::Equation1(size_t e, double T)
	//{
	//	const double* p = P[e];//current P for equation
	//	size_t s = e2s(e);//compute stage for b1Factor

	//	double Tau = (T - p[PTB]) / (p[PTM] - p[PTB]);
	//	double p1 = 1 / (1 + exp(p[PB2] - p[PB3] * Tau));
	//	double p2 = exp((Tau - 1) / p[PB4]);
	//	double Rt = p[PB1] * b1Factor[s] * (p1 - p2);

	//	return max(0.0, Rt);
	//}

	//double CForestTentCaterpillarEquations::Equation2(size_t e, double T)
	//{
	//	const double* p = P[e];//current P for equation
	//	size_t s = e2s(e);//compute stage for b1Factor

	//	double p1 = exp(-0.5*Square((T - p[PB2]) / p[PB3]));
	//	double Rt = p[PB1] * b1Factor[s] * p1;

	//	return max(0.0, Rt);
	//}

	//double CForestTentCaterpillarEquations::Equation3(size_t e, double T)
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

	//TT: air temperature [°C]
	double CForestTentCaterpillarEquations::GetGrayEggDevRate(size_t t, double T)
	{
		ASSERT(t < NB_TREES);
		enum TParam { α, қ, ᴩ, Δᵀ, Tм };
		static const double R[NB_GRAY_TREES][5] =
		{
			{0.3954, 21.1900, 0.2242, 0.7670, 18.5955},
			{0.4671, 21.5071, 0.2100, 0.6601, 18.9312}
		};

		double r = 0;
		if (T > 6.8)
		{
			double TT = T - 6.8;
			double ƫ = (R[t][Tм] - TT) / R[t][Δᵀ];
			r = max(0.0, R[t][α] * (pow(1 + R[t][қ] * exp(-R[t][ᴩ] * TT), -1) - exp(-ƫ)));
		}

		return r;
	}

	double CForestTentCaterpillarEquations::GetGrayEggRelDevRate(size_t t)const
	{
		ASSERT(t < NB_GRAY_TREES);
		enum TParam { α, β, ɤ };

		static const double F[NB_GRAY_TREES][3] =
		{
			{1.4126, 0.2541, 0.8253},
			{1.6611, 0.3232, 0.7709}
		};

		//double f = 1 - exp(-pow((r - F[t][ɤ])/ F[t][β], F[t][α]));

		double rr = m_randomGenerator.RandWeibull(F[t][α], F[t][β]) + F[t][ɤ];
		while (rr>1.8)
			rr = m_randomGenerator.RandWeibull(F[t][α], F[t][β]) + F[t][ɤ];

		return rr;
	}

	
	double CForestTentCaterpillarEquations::GetDevRate(size_t s, double T)
	{
		double r = 0;
		switch (s)
		{
		case L1:	r = 1.0/5.0; break;
		case L2:	r = 1.0/5.0; break;
		case L3:	r = 1.0/5.0; break;
		case L4:	r = 1.0/5.0; break;
		case L5:	r = 1.0/5.0; break;
		case PUPAE:	r = 1.0/5.0; break;
		case ADULT:	r = 1.0/5.0; break;//Live ~ 5 days after https://www.dec.ny.gov/docs/lands_forests_pdf/ftc02.pdf
		default: _ASSERTE(false);
		}

		return r;
	}

	//Daily development rate
	double CForestTentCaterpillarEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		double r = 0;

		switch (s)
		{
		case EGG:	r = GetGrayEggDevRate(TREMBLING_ASPEN, T); break;
		case L1:
		case L2:
		case L3:
		case L4:
		case L5:
		case PUPAE:
		case ADULT:	r = GetDevRate(s, T); break;
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);
		
		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 
	double CForestTentCaterpillarEquations::GetRelDevRate(size_t s)const
	{
		return 1;
	}

	double CForestTentCaterpillarEquations::GetRelativeDevRate(size_t s)const
	{

		double rr = 0;
		switch (s)
		{
		case EGG:	rr = GetGrayEggRelDevRate(TREMBLING_ASPEN); break;
		case L1:	rr = GetRelDevRate(s); break;
		case L2:	rr = GetRelDevRate(s); break;
		case L3:	rr = GetRelDevRate(s); break;
		case L4:	rr = GetRelDevRate(s); break;
		case L5:	rr = GetRelDevRate(s); break;
		case PUPAE:	rr = GetRelDevRate(s); break;
		case ADULT:	rr = 1; break;
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(rr) && _finite(rr));

		return rr;
	}

	//*****************************************************************************
	//
	double CForestTentCaterpillarEquations::GetFecondity()const
	{
		//after William(2006)
		//Forest tent caterpillar mating, oviposition, and adult congregation at town lights during a northern Minnesota outbreak
		//246 ± SD 66,
		//Baker (1972) 150-350
		double F = m_randomGenerator.RandNormal(246, 66);
		while(F<150 || F>350)
			F = m_randomGenerator.RandNormal(246, 66);

		return F;
	}
	
	
}