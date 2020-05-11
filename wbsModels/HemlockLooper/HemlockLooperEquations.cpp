//*****************************************************************************
// File: HLDevelopment.h
//
// Class: CHLDevelopment
//          
//
// Descrition: the CHLDevelopment can compute daily hemlock looper development rate
//             HemlockLooperEquations is an optimisation table lookup
//			   Base on docuement Régnière 2015
//*****************************************************************************
// 22/01/2016	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 22/04/2015	Rémi Saint-Amant	Add survival equations
// 17/04/2014	Jacques Regniere	enters new parameters for all life stages
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "ModelBase/WeatherBasedSimulation.h"
#include "HemlockLooperEquations.h"


using namespace std;
using namespace WBSF::HemlockLooper;

namespace WBSF
{
	//*****************************************************************************
	//CHLDevelopment class 

	//development rate parameters (6 stage, 6 parameters)
	const double HemlockLooperEquations::DEFAULT_P[NB_STAGES - 1][NB_PARAM] =
	{
	//   rho	  Ha      Hl      Tl      Hh	 Th
		//0.1022, 13.12, -44.36, 279.6, 37.01, 305.5, //SA HOBO
		0.1022, 13.22, -49.60, 279.6, 120.0, 303.8,	//Egg
		0.2160, 10.37, -70.62, 282.8, 128.4, 303.6,	//L1
		0.2720, 8.030, -70.79, 284.0, 216.9, 301.6,	//L2
		0.2610, 6.300, -54.96, 285.4, 110.4, 301.8,	//L3 
		0.1170, -3.09, -56.98, 287.1, 483.0, 298.1,	//L4 
		0.0770, 4.770, -56.00, 284.0, 100.0, 302.2,	//Pupa
	};

	//HOBO all
	//NbVal = 733	Bias = 0.14632	MAE = 9.98915	RMSE = 19.08965	CD = 0.77579	R² = 0.78558
	//rho = 0.10223  HA = 13.12449  HL = -44.35833  TL = 279.60077  HH = 37.01095  TH = 305.53867  Tmin = 2.36056

	//Weather stations hourly
	//NbVal = 310	Bias = 1.18581	MAE = 9.72516	RMSE = 18.21809	CD = 0.79174	R² = 0.80992
	//rho = 0.10215,HA = 13.19008 ,HL = -53.95094,TL = 279.59975,HH = 33.73600,TH = 303.56537

	//Weather stations daily
	//NbVal=   310	Bias= 0.78258	MAE=10.50323	RMSE=20.80570	CD= 0.72838	R²= 0.75146
	//rho = 0.10157  HA = 14.19774  HL = -162.36868  TL = 279.37969  HH = 33.55626  TH = 300.08384
	
	const double HemlockLooperEquations::RHO25_FACTOR[NB_STAGES - 1] = { 1.0, 0.9, 0.9, 0.9, 0.9, 0.9 }; //BY RSA 22-04-2017
	//const double HemlockLooperEquations::RHO25_FACTOR[NB_STAGES - 1] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

	

	HemlockLooperEquations::HemlockLooperEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
		for (size_t i = 0; i < NB_STAGES - 1; i++)
			for (size_t j = 0; j < NB_PARAM; j++)
				m_p[i][j] = DEFAULT_P[i][j];

		for (size_t i = 0; i < NB_STAGES - 1; i++)
			m_rho25Factor[i] = RHO25_FACTOR[i];

		for (size_t i = 0; i < NB_PARAM; i++)
			m_eggsParam[i] = DEFAULT_P[EGGS][i];
		
		
		m_Tlo = 3.35;//m_Tmin already exist in base class

		Init();
	}

	//s:	stage
	//L:	latitude [º]
	//T:	temperature [ºC] 
	//out:	development rate in function of stage and latitude
	double HemlockLooperEquations::GetRate(size_t s, double L, double T)const
	{
		//Equation [4] and [6]
		//                             egg     larval   pupae   adult
		static const double b[4] = { 0.00481, 0.0601, 0.02094, 0.0308 };

		static const double L0 = 50.633;
		size_t e = (s == EGGS) ? 0 : (s < PUPAE) ? 1 : (s < ADULTS) ? 2 : 3;

		//get rate from table lookup and adjust it in function of latitude
		double rº = CEquationTableLookup::GetRate(s, T);
		double rᴸ = rº * (1 + b[e] * (L - L0));

		return rᴸ;
	}

	//s:	stage
	//T:	temperature [ºC] 
	//out:	relative development rate in function of stage and temperature
	double HemlockLooperEquations::ComputeRate(size_t s, double T)const
	{
		assert(s < NB_STAGES);
		assert(T >= 0);


		double r = 0;

		if (s == ADULTS) //Adult longevity
		{
			//Equation []
			static const double a = 233.2;
			static const double b = -0.846;
			if (T>0)
				r = 1.0 / (a * pow(T, b));
		}
		else if (s == EGGS) //Eggs
		{
			enum{ rho, Hᴬ, Hᴸ, Tᴸ, Hᴴ, Tᴴ };
			static const double R = 1.987E-3;

			if (T > m_Tlo)
			{
				double K = T + 273.0;
				double rho25 = m_rho25Factor[EGGS] * m_eggsParam[rho];
				double num = rho25*K / 298.0*exp(m_eggsParam[Hᴬ] / R*(1 / 298.0 - 1.0 / K));
				double den1 = exp(m_eggsParam[Hᴸ] / R*(1 / m_eggsParam[Tᴸ] - 1 / K));
				double den2 = exp(m_eggsParam[Hᴴ] / R*(1 / m_eggsParam[Tᴴ] - 1 / K));

				r = num / (1 + den1 + den2);
			}
		}
		else
		{
			enum{ rho, Hᴬ, Hᴸ, Tᴸ, Hᴴ, Tᴴ };
			static const double R = 1.987E-3;

			if (T > 3.35)
			{
				double K = T + 273.0;
				double rho25 = m_rho25Factor[s] * m_p[s][rho];
				double num = rho25*K / 298.0*exp(m_p[s][Hᴬ] / R*(1 / 298.0 - 1.0 / K));
				double den1 = exp(m_p[s][Hᴸ] / R*(1 / m_p[s][Tᴸ] - 1 / K));
				double den2 = exp(m_p[s][Hᴴ] / R*(1 / m_p[s][Tᴴ] - 1 / K));

				r = num / (1 + den1 + den2);
			}
		}

		assert(isfinite(r));
		assert(!isnan(r));

		return max(0.0, min(1.0, r));//This is daily rate, if time step smaller, must be multyiplied by time step...
	}


	//s:	stage
	//sex:	sex(male=0, female=1)
	//out:	relative development rate in function of stage and sex
	double HemlockLooperEquations::GetRelativeRate(size_t s, size_t sex)const
	{
		ASSERT(s < NB_STAGES);
		ASSERT(sex == MALE || sex == FEMALE);

		//Variability of egg development rate was 0.1172, and was multiplied by 0.5 to reflect the temperature dependence of this variability (smaller at cooler temperature)
		//						       Egg       L1     L2     L3     L4   Pupae  Adult
		const double σᵋ[NB_STAGES] = { 0.0469, 0.127, 0.246, 0.172, 0.150, 0.068, 0.314 };
		const double Δρ[NB_SEX][NB_STAGES] =
		{
			{ 0, 0, 0, 0, 0, 0, 0 },//no adjustement for male
			{ 0, -0.008, -0.016, -0.018, -0.020, 0.005, 0 } //Adjust development rate of females
		};
		return (1 + Δρ[sex][s])*m_randomGenerator.RandUnbiasedLogNormal(0, σᵋ[s]);
	}

	//T:	temperature [ºC]
	//out:	pre-diapause development rate
	double HemlockLooperEquations::GetPreDiapauseRate(double T)const
	{
		static const double a = 4932.96;
		static const double b = -2.168;
		//Question pour Remi: Si tu calcules un TAUX de développement ici, j'ai changé pour 1/(a*pow(T,b))
		//Réponse: OK
		//	return T>0 ? a*pow(T, b) : 0;
		return T > 0 ? 1 / (a*pow(T, b)) : 0;
	}

	//out:	pre-diapause relative development rate
	double HemlockLooperEquations::GetPreDiapauseRelativeRate()const
	{
		return m_randomGenerator.RandUnbiasedLogNormal(0, 0.285);
	}

	//L:	latitude [º]
	//out:	ratio of larvae the will have a L5 stage
	double HemlockLooperEquations::GetL5Ratio(size_t sex, double L)const
	{
		static const double a[2] = { -31.22, -30.14 };
		static const double b[2] = { 9.9, 9.9 };
		//Question pour Remi: ici c'est un logit. Je crois que la transformation en proportion (entre 0 et 1) devrait être:
		//Réponse: OK
		//	double l = log(a[sex] + b[sex] * L);
		//	return max(0.0, min(1.0, l / (1.0 + l)));
		double l = a[sex] + b[sex] * L;
		return 1 / (1 + exp(-L));
	}

	//***********************************************************************************

	//L:	latitude in [º]
	//out:	individual's fecundity [eggs]
	double CHLOviposition::GetFecundity(double L)const
	{
		double F = 1075.2 - 17.23*L;
		return m_randomGenerator.RandNormal(F, 32.07);//173.18;
	}


	//T:	temperature [ºC]
	//Fº:	initial individual fecondity [eggs]
	//Fᵗ:	remaining fecundity  [eggs]
	//out:	oviposition rate as a function of temperature [eggs/day]
	double CHLOviposition::GetRate(double T, double Fº, double Fᵗ)const
	{
		static const double a = -0.053;
		static const double b = 6.6E-5;
		static const double c = 0.094;
		static const double d = 0.118;
		static const double e = 173.2;


		double Eᵗ = 0.0;
		if (Fᵗ > 0)
		{
			double F = (e / Fº)*T*(a + b * Fᵗ + c / pow(Fᵗ, d));
			Eᵗ = Fᵗ*max(0.0, min(1.0, F));
		}

		return Eᵗ;
	}




	//***********************************************************************************
	//T:	temperature at time step t [ºC]
	//out:	energy loss per day at this temperature
	double CHLSurvival::SenergyʃT(double T)
	{
		static const double α² = 8.341;
		return T > 0 ? pow(T, α²) : 0;
	}

	//ʃT:	integral of temperature exposure
	double CHLSurvival::Senergy(double ʃT)
	{
		static const double αº = 0.078;
		static const double α¹ = -1.29E-13;

		return 1.0 / (1.0 + exp(-(αº + α¹*ʃT)));
	}

	//Tmin: minimum temperature at witch insect was exposed [ºC]
	//out:	cold survival
	double CHLSurvival::Scold(double Tmin)
	{
		static const double β0 = -24.28;
		static const double β¹ = -2.401;
		static const double β² = -0.05;
		if (Tmin > -25)
			Tmin = -25;

		return 1.0 / (1.0 + exp(-(β0 + β¹*Tmin + β²*Tmin*Tmin)));
	}


	//T:	temperature at time step t [ºC]
	//dt:	time step duration (day)
	//out:	hatch survival for this time step
	double CHLSurvival::Shatch(double T, double dt)
	{
		static const double β0 = 7.151;
		static const double β¹ = -0.558;
		static const double β² = 0.0436;
		static const double β³ = -0.00108;
		return T > 0 ? pow(1.0 + exp(-(β0 + β¹*T + β²*T*T + β³*T*T*T)), -dt) : 1;
	}

	//L:	latitude in º
	//out:	weight survival
	double CHLSurvival::Sweight(double L)
	{
		double W = 0.01066*L - 0.3243;
		return max(0.0, min(1.0, 10 * W - 1.342));
	}



	//L:	latitude in º
	//Sh:	hatch survival
	//ʃT:	integral of temperature exposition
	//Tmin: minimum temperature at witch insect was exposed [ºC]
	double CHLSurvival::GetEggSurvival(double L, double Sh, double ʃT, double Tmin)
	{
		ASSERT(Sh >= 0 && Sh <= 1);

		static const double ƙ = 3.046;

		double Sw = Sweight(L);
		double Se = Senergy(ʃT);
		double Sc = Scold(Tmin);
		ASSERT(Sw >= 0 && Sw <= 1);
		ASSERT(Se >= 0 && Se <= 1);
		ASSERT(Sc >= 0 && Sc <= 1);

		return max(0.0, min(1.0, ƙ*Sw*Se*Sc*Sh));
	}

	//out:	daily attrition rate
	double CHLSurvival::GetWinterSurvival()const
	{
		return m_randomGenerator.Randu(); //for overwinter survival
	}

	//out:	daily attrition rate
	double CHLSurvival::GetAttrition()
	{
		return 0.995566;
	};


}