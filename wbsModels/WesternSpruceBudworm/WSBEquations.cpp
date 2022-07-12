//*****************************************************************************
// File: WSBDevelopment.h
//
// Class: CWSBDevelopment
//          
//
// Descrition: the CWSBDevelopment can compute daily western spruce budworm devlopement rate
//             CWSBTableLookup is an optimisation table lookup
//*****************************************************************************
// 21/01/2016   Rémi Saint-Amant	Update with BioSIM 11.0
// 12/10/2012   Rémi Saint-Amant    Update with new template
// 28/01/2011	Rémi Saint-Amant    Add Attrition 
// 23/01/2009   Rémi Saint-Amant    Creation from Jacques code
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "WSBEquations.h"




using namespace std;

namespace WBSF
{
	//*****************************************************************************
	//CWSBDevelopment class 


	//development rate parameters (10 stage, 6 parameters)
	const double CWSBTableLookup::DEFAULT_P[NB_STAGES][6] =
	{//Revised 2011-08-06from SAS output (JR)	
		//rho25	HA		HL		TL		HH		TH
		0.158, 11443, -59715., +283.900, 99958, 308.2,	//Egg
		0.217, 18186, -58752., +276.700, 99941, 300.7,	//L2o
		0.173, 12707, -50000., +277.400, 98106, 307.8,	//L2
		0.263,  9738, -100000, +285.100, 91262, 306.7,	//L3
		0.330, 13271, -99659., +283.700, 91301, 306.2,	//L4
		0.303, 13031, -99659., +273.000, 91292, 305.5,	//L5
		0.187, 12651, -99659., +274.000, 91276, 304.0,	//L6
		0.288, 25810, -23000., +296.500, 59590, 306.4,	//Pupa
		120.0, -13.2, +0.5669, -0.00854, 0.000, 0.000	//Adult longevity (3RD DEGREE POLYNOMIAL ON TIME)
	};


	//L2 adjustment (final model)
	const double CWSBTableLookup::RHO25_FACTOR[NB_STAGES] = { 1, 1, 2.62, 1, 1, 1, 1, 1, 1 };

	CWSBTableLookup::CWSBTableLookup(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
		for (int i = 0; i < NB_STAGES; i++)
			for (int j = 0; j < NB_PARAMETER; j++)
				m_p[i][j] = DEFAULT_P[i][j];

		for (int i = 0; i < NB_STAGES; i++)
			m_rho25Factor[i] = RHO25_FACTOR[i];

		Init();
	}


	double CWSBTableLookup::ComputeRate(size_t s, double T)const
	{
		double Rt = 0;

		if (T <= 0)
			return 0;


		if (s == 8) //Maximum 30 day longevity at lower T, but death in 1 day at T<=0
		{
			Rt = m_rho25Factor[s] / min(30.0, max(1.0, (m_p[s][0] + m_p[s][1] * T + m_p[s][2] * T*T + m_p[s][3] * T*T*T)));
		}
		else
		{
			double TK = T + 273.0;
			double rho25 = m_rho25Factor[s] * m_p[s][0];
			double num = rho25*TK / 298.0*exp(m_p[s][1] / 1.987*(1 / 298.0 - 1 / TK));
			double den1 = exp(m_p[s][2] / 1.987*(1 / m_p[s][3] - 1 / TK));
			double den2 = exp(m_p[s][4] / 1.987*(1 / m_p[s][5] - 1 / TK));

			Rt = num / (1 + den1 + den2);
		}

		return min(1.0, max(0.0, Rt)); //This is daily rate, if time step smaller, must be multyiplied by time step...
	}

	//***********************************************************************************
	//
	//Revised 2011-03-07 from SAS output (JR)	
	const double CWSBRelativeDevRate::V[NB_STAGES] =
	//Egg    OvL2   FeedL2   L3     L4     L5     L6    Pupa   Adult
	{ 0.063, 0.242, 0.229, 0.428, 0.289, 0.254, 0.252, 0.101, 0.279 };

	const double CWSBRelativeDevRate::S[2][NB_STAGES] =
		//Revised 2011-03-07 from SAS output (JR)	
	{//Egg,    OvL2, FeedL2,     L3,     L4,      L5,      L6,   Pupa, Adult
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, -0.017, -0.039, 0.013, 0 } //ASdjust development rate of females
	};

	double CWSBRelativeDevRate::GetRate(size_t s, size_t sex)
	{
		_ASSERTE(s >= 0 && s < NB_STAGES);
		ASSERT(sex == 0 || sex == 1);

		return (1 + S[sex][s])*RandLogNormal(0, V[s]);
	}

	//***********************************************************************************

	const double CWSBAttrition::p[NB_STAGES][3] =
	{
		+0.0000, +0.00000, +0.00000,//Egg  
		+0.0000, +0.00000, +0.00000,//OW
		-6.3464, +0.10440, -0.00091,//L2
		-5.3884, +0.05502, +0.00000,//L3
		-5.6125, +0.06048, +0.00000,//L4
		-7.2000, +0.00618, +0.00288,//L5
		-2.0299, -0.85730, +0.02766,//L6
		+8.3397, -1.57860, +0.04145,//PUPAE
		+0.0000, +0.00000, +0.00000 //Adults (not used, nonsensical)  
	};

	double CWSBAttrition::GetRate(size_t s, double Tin)
	{
		_ASSERTE(s >= 0 && s < NB_STAGES);

		double T = max(0., Tin);
		double att = 1 / (1 + exp(p[s][0] + p[s][1] * T + p[s][2] * T*T));

		return att;
	};

	//***********************************************************************************
	const double CWSBOviposition::X0 = -0.9952;
	const double CWSBOviposition::X1 = -0.0345;
	const double CWSBOviposition::X2 = +0.4425;
	const double CWSBOviposition::X3 = +0.3987;

	double CWSBOviposition::GetRate(double T, double f)
	{
		double l = 0.0;
		if (f>0)
			l = max(0.0, min(1.0, X0 + X1*T + X2*pow(T, 0.5) + X3 / f));

		return l;
	}

}