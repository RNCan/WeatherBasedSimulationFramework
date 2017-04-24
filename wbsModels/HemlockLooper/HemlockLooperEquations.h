//*****************************************************************************
// File: HLDevelopment.h
//
// Class: CHLDevelopment
//          
//
// Descrition: the CHLDevelopment can compute hourly Hemlock Looper devlopement rate
//             HemlockLooperEquations is an optimisation table lookup
//*****************************************************************************
#pragma once

//#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace HemlockLooper
	{
		enum TStages{ EGGS, L1, L2, L3, L4, PUPAE, ADULTS, DEAD_ADULTS, NB_STAGES = DEAD_ADULTS };
	}



	//***********************************************************************************
	//Hemlock Looper Equation

	class HemlockLooperEquations : public CEquationTableLookup
	{
	public:

		enum TParameter{ RHO25, HA, HL, TL, HH, TH, NB_PARAM };
		HemlockLooperEquations(const CRandomGenerator& RG);

		void SetRho25(double rho25Factor[HemlockLooper::NB_STAGES])
		{
			bool bHaveChange = false;
			for (size_t s = 0; s < HemlockLooper::NB_STAGES; s++)
			{
				if (fabs(m_rho25Factor[s] - rho25Factor[s]) > 0.0000001)
				{
					bHaveChange = true;
					m_rho25Factor[s] = rho25Factor[s];
				}
			}

			if (bHaveChange)
				Init(true);
		}

		void SetEggParam(double Tlo, double eggsParam[NB_PARAM])
		{
			bool bHaveChange = false;
			for (size_t s = 0; s < NB_PARAM; s++)
			{
				if (fabs(eggsParam[s] - m_eggsParam[s]) > 0.0000001)
				{
					bHaveChange = true;
					m_eggsParam[s] = eggsParam[s];
				}
			}

			if (Tlo != m_Tlo)
			{
				bHaveChange = true;
				m_Tlo = Tlo;
			}

			if (bHaveChange)
				Init(true);
		}

		double GetRate(size_t s, double L, double T)const;

		double GetRelativeRate(size_t s, size_t sex)const;
		double GetPreDiapauseRate(double T)const;
		double GetPreDiapauseRelativeRate()const;
		double GetL5Ratio(size_t sex, double L)const;

	protected:

		double m_p[HemlockLooper::NB_STAGES - 1][NB_PARAM];
		double m_rho25Factor[HemlockLooper::NB_STAGES - 1];
		double m_eggsParam[NB_PARAM];
		double m_Tlo;

		virtual double ComputeRate(size_t s, double T)const;

		static const double DEFAULT_P[HemlockLooper::NB_STAGES - 1][NB_PARAM];
		static const double RHO25_FACTOR[HemlockLooper::NB_STAGES - 1];
		
	};





	//***********************************************************************************
	//CCHLOviposition

	class CHLOviposition
	{
	public:



		CHLOviposition(const CRandomGenerator& RG) :
			m_randomGenerator(RG)
		{}

		double GetFecundity(double L)const;
		double GetRelativeRate(size_t s)const;
		double GetRate(double T, double F0, double f)const;


	protected:



		const CRandomGenerator& m_randomGenerator;
	};



	//***********************************************************************************
	//CHLSurvival

	class CHLSurvival
	{
	public:

		CHLSurvival(const CRandomGenerator& RG) :
			m_randomGenerator(RG)
		{}

		static double GetAttrition();
		static double GetEggSurvival(double L, double Sh, double ʃT, double Tmin);


		double GetWinterSurvival()const;


		static double Scold(double Tmin);
		static double Sweight(double L);
		static double Shatch(double T, double r);
		static double Senergy(double ʃT);
		static double SenergyʃT(double T);


	protected:

		const CRandomGenerator& m_randomGenerator;

	};

}