//*****************************************************************************
// File: SBBDevelopment.h
//
// Class: CSBBDevelopment
//          
//
// Descrition: the CSBBDevelopment can compute daily Spruce Bark Bettle
//			   devlopement rate

//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "BugDevelopmentTable.h"
#include <array>

//OVIPOSITING_ADULT
//PRE_OVIP_ADULT, OVIP_ADULT
enum TStages{ EGG, L1, L2, L3, PUPAE, TENERAL_ADULT, ADULT, DEAD_ADULT, NB_STAGES=DEAD_ADULT };

//***********************************************************************************
//CSBBDevelopment
class CSBBDevelopmentTable: public CBugDevelopmentTable
{
public:

	enum TParameter{P_ALPHA,P_BETA,P_GAMMA,P_DTL,P_TO,P_DTU,P_TMAX, P_K, NB_PARAMETER};
	static const double DEFAULT_P[NB_STAGES][NB_PARAMETER];
	static const double V[NB_STAGES];//variance of relative development rates



	CSBBDevelopmentTable();

	//void SetRho25(double rho25Factor[NB_STAGES])
	//{
	//	bool bForceInit=false;
	//	for(int s=0; s<NB_STAGES; s++)
	//	{
	//		if( fabs(m_rho25Factor[s]-rho25Factor[s]) > 0.0000001)
	//		{
	//			bForceInit=true;
	//			m_rho25Factor[s]=rho25Factor[s];
	//		}
	//	}

	//	//if( bForceInit )
	//		//Reinit();

	//	if( bForceInit )
	//		Init(bForceInit);
	//}

	
	virtual double ComputeRate(size_t e, double t)const;
	static double GetRelativeRate(const CRandomGenerator& RG, int s, int sex);
	static double GetDayLengthFactor(int s, double t, double dayLength);
	static double GetTo(int s);


protected:
	
	double m_p[NB_STAGES][NB_PARAMETER];
	//double m_rho25Factor[NB_STAGES];

	
};



//***********************************************************************************
//CSBBAttrition
/*
class CSBBAttrition
{
public:

	static double GetRate(int s, double T);

private:
	static const double p[NB_STAGES][3];
};

*/
//***********************************************************************************
//CCSBBOviposition

class CSBBOviposition
{
public:

	//PRE_OVIPOSITION_ADULT
	//ovipositing adult include pre-oviposition
	enum TEquation{DEVELOPEMENT_RATE, BROOD_RATE, NB_EQUATIONS};
	enum TParameter{P_ALPHA,P_BETA,P_GAMMA,P_DTL,P_TO,P_DTU,P_TMAX, NB_PARAMETER};
	static double GetRate(int e, double T);

	static double GetRelativeRate(const CRandomGenerator& RG, int e);
	static double GetRelativeQuartile(double RR, int e=DEVELOPEMENT_RATE);

private:
	
	static const double DEFAULT_P[NB_EQUATIONS][NB_PARAMETER];
	static const double V[NB_EQUATIONS];//variance of relative oviposition rates (development and brood)
};



//Diapause

class CDiapause
{
public:

//protected:

	static const double MTS_CLASS_20[33][2];
};
