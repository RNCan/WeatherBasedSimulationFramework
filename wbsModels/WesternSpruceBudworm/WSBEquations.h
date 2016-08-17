//*****************************************************************************
// File: WSBDevelopment.h
//
// Class: CWSBDevelopment
//          
//
// Descrition: the CWSBDevelopment can compute daily Western Spruce Budworm 
//			   devlopement rate
//             CWSBTableLookup is an optimisation table lookup
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase\EquationTableLookup.h"


namespace WBSF
{

	enum TStages{ EGG, L2o, L2, L3, L4, L5, L6, PUPAE, ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT };

	//***********************************************************************************
	//CWSBDevelopment
	class CWSBTableLookup : public CEquationTableLookup
	{
	public:

		enum TParameter{ RHO25, HA, HL, TL, HH, TH, NB_PARAMETER };
		CWSBTableLookup(const CRandomGenerator& RG);

		void SetRho25(double rho25Factor[NB_STAGES])
		{
			bool bForceInit = false;
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				if (fabs(m_rho25Factor[s] - rho25Factor[s]) > 0.0000001)
				{
					bForceInit = true;
					m_rho25Factor[s] = rho25Factor[s];
				}
			}

			if (bForceInit)
				Init(bForceInit);
		}


	protected:

		double m_p[NB_STAGES][NB_PARAMETER];
		double m_rho25Factor[NB_STAGES];

		
		virtual double ComputeRate(size_t s, double T)const;
		static const double DEFAULT_P[NB_STAGES][NB_PARAMETER];
		static const double RHO25_FACTOR[NB_STAGES];
	};



	//***********************************************************************************
	//CWSBRelativeDevRate

	class CWSBRelativeDevRate
	{
	public:

		static double GetRate(size_t s, size_t sex);

	private:

		static const double V[NB_STAGES];
		static const double S[2][NB_STAGES];
	};
	//***********************************************************************************
	//CWSBAttrition

	class CWSBAttrition
	{
	public:

		static double GetRate(size_t s, double T);

	private:
		static const double p[NB_STAGES][3];
	};


	//***********************************************************************************
	//CCWSBOviposition

	class CWSBOviposition
	{
	public:

		static double GetRate(double T, double f);

	private:
		static const double X0;
		static const double X1;
		static const double X2;
		static const double X3;
	};

}