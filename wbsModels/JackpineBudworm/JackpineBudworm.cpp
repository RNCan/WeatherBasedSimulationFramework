//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cps in the current
//   directory is used
//***********************************************************

//**************************************************************************
// From:   CEDAR::"LYSYK@ABRSLE.AGR.CA" 31-JAN-1994 13:22:15.58
//To:     cfl::regniere
//CC:
//Subj:   jackpine budworm model

//Jacques.....here is the code for the JPBW model.

//The code is based on a 9 stage x 30 day matrix. The nine stages are:
//1 = overwintering larvae,
//2 - 7 = respective feeding instars,
//8 = pupae,
//9 = adults,

//It reads a current.cps (BUDWORM.SET) file to initialize some conditions.
//The first line is the name of the input file, the second is the name of the
//output file, the third is the density of overwintering insects, the fourth
//line contains dummy variables to use a) linear equations b) bud microclimate
//and c) bark microclimate corrections.

//The input file contains year, julian date, minimum and maximum temperatures.
//The output file is the numbers in each stage on a given date. Note that there
//is no mortality modelled, so initializing with 100 gives you the percent in
//each stage after all spring emergence has occured. One glitch with the adults...
//once the population is entirely adults, the numbers apparentlty decrease
//after 30 days pass in the matrix due to loss of individuals in the matrix.
//This isn't really important, its just one of those fuuny things that happens
//that I never worried about.

//The dummy variables are important....

//1. ILIN = 0 uses nonlinear rate equations
//   ILIN = 1 uses linear rate equations

//2. IBUD = 0 uses no bud temperature corrections for feeding stages
//   IBUD = 1 uses bud temperature corrections for feeding stages

//3. IBARK = 0 uses no bark temperature corrections for overwintering larvae
//   IBARK = 1 uses bark temperature corrections for overwinteriong larvae.

//This made the temperature variables a little cumbersome.

//TK = temperature in degrees Kelvin (By the way, I swore off the WAGNER rate
//equations). TK1 is bark temperature, TK2 is bud temperatures.

//Have fun.......the model is documented in Lysyk, 1989, Can. Entomol.
//121: 373 - 387.
//***********************************************************************
//************** M O D I F I C A T I O N S   L O G ********************
// 20/09/2016	2.3.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 18/04/2016	2.2.0	Rémi Saint-Amant	Use WBSF with BioSIM11
// 08/07/2010			Rémi Saint-Amant	New Compile 
// 26/04/2006			Rémi Saint-Amant	translate from fortran and used BioSIMModelBase
// 09/01/1995			Jacque Régniere		creation


//**********************************************************************
#include "JackPineBudworm.h"
#include "ModelBase/EntryPoint.h"
#include <math.h>
#include <crtdbg.h>


namespace WBSF
{


	using namespace HOURLY_DATA;


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CJackpineModel::CreateObject);

	//must be 1 because the method Devel don't support other time step
	static const short TIME_STEP = 1;


	//put your need in the constructor
	//these flag must be synchronized with the model interface
	//in order:
	CJackpineModel::CJackpineModel()
	{
		// initialise your variable here (optionnal)
		//NB_INPUT_PARAMETER is use to determine if the dll
		//use the same number of parameter than the model interface
		NB_INPUT_PARAMETER = 4;
		VERSION = "2.3.0 (2016)";
	}

	CJackpineModel::~CJackpineModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CJackpineModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg message;

		m_modelType = parameters[0].GetInt();
		m_bLinear = parameters[1].GetBool();
		m_bUseBud = parameters[2].GetBool();
		m_bUseBark = parameters[3].GetBool();

		return message;
	}

	void CJackpineModel::Reset()
	{
		for (int i = 0; i < NB_INSTAR; i++)
			for (int j = 0; j < NB_CLASS; j++)
				FOLD[i][j] = 0;

		FOLDE = 0;
	}

	//This method is call to compute solution
	ERMsg CJackpineModel::OnExecuteDaily()
	{
		ERMsg message;

		switch (m_modelType)
		{
		case THERRIEN:	ExecuteTherrien(); break;
		case LYSYK:		ExecuteLysyk(); break;
		default: _ASSERTE(false);
		}

		return message;
	}

	enum TLysykOutput{ O_NB_OVERWIN_BUGS, O_L2, O_L3, O_L4, O_L5, O_L6, O_L7, O_PUPEA, O_ALDULT, O_AI, NB_OUTPUT };
	typedef CModelStatVectorTemplate<NB_OUTPUT> CJackPineStatVector;

	void CJackpineModel::ExecuteLysyk()
	{

		CJackPineStatVector stat(m_weather.GetEntireTPeriod());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{

			Reset();//reset object
			double age[NB_INSTAR][NB_CLASS] = { 0 };
			double nbBugs[NB_INSTAR][NB_CLASS] = { 0 };

			//------------------------------------------------------------
			//    INITIALIZE age OF OVERWINTERING LARVAE                  
			//------------------------------------------------------------
			double overWinAge = 0;
			double nbOverWinBugs = 100;

			CTPeriod p = m_weather[y].GetEntireTPeriod();
			for (size_t d = 0; d < m_weather[y].GetNbDays(); d++)
			{
				
				//------------------------------------------------------------
				//            generate hourly temperature regime               
				//------------------------------------------------------------
				CDailyWaveVector t;
				m_weather[y].GetDay(d).GetAllenWave(t, 12, TIME_STEP, 0);


				//------------------------------------------------------------
				//       Compute development rate and sum to find the         
				//       physiological age accumulation for overwintering     
				//       and active stages.                                   
				//------------------------------------------------------------
				double overWinDev = 0;
				double stageDev[NB_INSTAR] = { 0 };
				Devel(t, overWinDev, stageDev);

				//------------------------------------------------------------
				//    calculate the physiological age of the overwintering    
				//    larvae.                                                  
				//------------------------------------------------------------
				overWinAge += overWinDev;

				//------------------------------------------------------------
				//    calculate the probability of emergence                  
				//------------------------------------------------------------
				double emergeProb = GetEmergeProb(overWinAge);

				//------------------------------------------------------------
				//    Add total development completed today to physiological  
				//    age matrix. The age matrix is age[i][j] where i=number  
				//    of instars and j=number of classes per stage. The       
				//    physiological age of each cohort is also incremented.   


				// est-ce encore bon???		
				//    each instar has the age of the first cohort initialized 
				//    according to the mean age on the physiological age      
				//    scale stored in the vector u[i].                        
				//------------------------------------------------------------
				for (int i = L2; i < NB_INSTAR; i++)
				{
					//The first class have always zero developement
					_ASSERTE(age[i][0] == 0);
					for (int j = NB_CLASS - 1; j >= 1; j--)
					{
						age[i][j] = age[i][j - 1] + stageDev[i];
					}
				}

				//------------------------------------------------------------
				//    Calculate the probability of transfer matrix, which     
				//    is called transferProb(NB_INSTAR, NB_CLASS) using the subroutine ComputeTransferProb. 
				//------------------------------------------------------------
				double transferProb[NB_INSTAR][NB_CLASS] = { 0 };
				ComputeTransferProb(age, transferProb);

				//------------------------------------------------------------
				//    This is a space for an as yet undefined mortality       
				//    routine.                                                
				//------------------------------------------------------------


				//------------------------------------------------------------
				//    calculate the number of each age group of stage i       
				//    which moults to stage i+1, nbBugsTrans[i][j].  
				//------------------------------------------------------------
				double nbBugsTrans[NB_INSTAR][NB_CLASS] = { 0 };
				for (int i = L2; i < NB_INSTAR; i++)
					for (int j = 0; j < NB_CLASS; j++)
						nbBugsTrans[i][j] = nbBugs[i][j] * transferProb[i][j];


				//------------------------------------------------------------
				//    Increment the numbers matrix.                           
				//------------------------------------------------------------
				for (int i = L2; i < NB_INSTAR; i++)
				{
					for (int j = NB_CLASS - 1; j >= 1; j--)
					{
						nbBugs[i][j] = nbBugs[i][j - 1] - nbBugsTrans[i][j - 1];
					}
				}

				//------------------------------------------------------------
				//   Calculate the number lost by transfer, and use this to   
				//   initialize the number entering the next stage.           
				//------------------------------------------------------------
				double nbBugsLost[NB_INSTAR] = { 0 };

				for (int i = L2; i < NB_INSTAR; i++)
				{
					for (int j = 0; j < NB_CLASS; j++)
					{
						nbBugsLost[i] += nbBugsTrans[i][j];
					}
				}

				for (int i = L3; i < NB_INSTAR; i++)
					nbBugs[i][0] = nbBugsLost[i - 1];


				//--------------------------------------------------------------
				//    Calculate the number of insects which emerge from         
				//    diapause and use these to initialize the matrix of        
				//    active feeders. nbOverWinBugs is the number left in overwintering
				//--------------------------------------------------------------
				nbBugs[L2][0] = emergeProb * 100;
				nbOverWinBugs -= nbBugs[L2][0];

				//------------------------------------------------------------
				//    Calculate the total numbers in each stage i, (total[i]) 
				//------------------------------------------------------------
				double total[NB_INSTAR] = { 0 };
				for (int i = L2; i < NB_INSTAR; i++)
				{
					for (int j = 0; j < NB_CLASS; j++)
					{
						total[i] += nbBugs[i][j];
					}
				}

				//------------------------------------------------------------
				//    Calculate the average instar
				//------------------------------------------------------------
				double LY_AI = GetLY_AI(total);
				if (nbOverWinBugs >= 100)
					LY_AI = 2;
				//------------------------------------------------------------
				//    output results                                          
				//------------------------------------------------------------
				CTRef TRef = p.Begin() + d;
				stat[TRef][O_NB_OVERWIN_BUGS] = nbOverWinBugs;
				for (int i = L2; i < NB_INSTAR; i++)
					stat[TRef][O_L2 + i - L2] = total[i];

				stat[TRef][O_AI] = LY_AI;
			}//for all days
		}//for year

		SetOutput(stat);
	}

	//------------------------------------------------------------
	//    compute ddays using allen (1976) sine wave method       
	//    then life stage frequencies and ai with therrien's      
	//    parameter values                                        
	//------------------------------------------------------------
	void CJackpineModel::ExecuteTherrien()
	{
		//------------------------------------------------------------C
		//    BASE TEMPERATURE FOR DEGREEDAY CALCULATIONS             C
		//------------------------------------------------------------C

		static const double BASE_TEMP = 2.0;

		CJackPineStatVector stat(m_weather.GetEntireTPeriod());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			double DDAYS = 0.0;
			for (size_t d = 1; d < m_weather[y].GetNbDays() - 1; d++)
			{
				const CWeatherDay& day = m_weather[y].GetDay(d);
				DDAYS = DDAYS + Allen(day[H_TMAX2][MEAN], day[H_TMIN2][MEAN], BASE_TEMP);

				//a for 1 base and 1 for overWintering
				double PINS[NB_INSTAR + 1] = { 0 };
				double AI = 0;
				Therrien(DDAYS, PINS, AI);

				CTRef TRef = p.Begin() + d;
				for (size_t i = OVERWINDEV; i < NB_INSTAR; i++)
					stat[TRef][O_NB_OVERWIN_BUGS + i - OVERWINDEV] = PINS[i + 1] * 100;

				stat[TRef][O_AI] = AI;
			}
		}

		SetOutput(stat);
	}






	double CJackpineModel::GetLY_AI(const double total[NB_INSTAR])const
	{
		double LY_AI = 0;

		double sum = 0;
		for (int i = L2; i < NB_INSTAR; i++)
			sum += total[i];

		if (sum > 0)
		{
			_ASSERTE(L2 == 0);
			for (int i = L2; i < NB_INSTAR; i++)
			{
				double prop = total[i] / sum;
				LY_AI += (i + 2)*prop;
			}
		}
		else LY_AI = 9;

		return std::max(2.0, LY_AI);
	}


	//------------------------------------------------------------
	//  This subroutine is based on a half-cosine function which  
	//  approxiamates daily temperature fluctuations by           
	//  interpolation between maxima and minima. the functions    
	//  are found in regniere, j. 1983. can. entomol. 114: 811-   
	//  825.                                                      
	//------------------------------------------------------------
	/*void CJackpineModel::TCycle(double TMAX[3], double TMIN[3], double TARRAY[24])
	{
	static const double TCOS[24] =
	{
	-.3827,-.5556,-.7071,-.8315,-.9239,-.9808,-1.0,.9239,
	.7071,.3827,0.0,-.3829,-.7071,-.9238,-1.0,.9808,.9239,.8315,
	.7071,.5556,.3827,.1951,0.0,-.1951
	};

	for( int N=0; N<24; N++)
	{
	if( N>=0 && N <=6)//       FROM 0:00 TO 6:00 AM
	{
	//       WHERE TCOS[N] = COS(3.14159*(TIME+10.)/16.)
	//       AND TIME <= 6
	TARRAY[N] = (TMAX[0]+TMIN[1])/2. + (TMAX[0]-TMIN[1])/2. * TCOS[N];
	}
	else if( N>=7 && N <=14)//       FROM 6:00 TO 14:00 PM
	{
	//       WHERE TCOS[N] = COS(3.14159*(TIME-6.)/8.)
	//       AND 6 < TIME <=14

	TARRAY[N] = (TMAX[1]+TMIN[1])/2. - (TMAX[1]-TMIN[1])/2. * TCOS[N];
	}
	else//       FROM 14:00 TO 24:00 PM
	{
	//       WHERE TCOS[N] = COS(3.14159*(TIME-14.)/16.)
	//       AND 14 < TIME

	TARRAY[N] = (TMAX[1]+TMIN[2])/2. + (TMAX[1]-TMIN[2])/2. * TCOS[N];
	}
	}
	}
	*/

	double CJackpineModel::GetRate(double TK, int stage)
	{
		_ASSERTE(stage >= OVERWINDEV && stage <= PUPEA);

		//all variable begin with overwintering ans end at PUPEA
		static const double A[8] = { -0.0408, -0.2396, -0.2948, -0.2041, -0.1667, -0.1051, -0.0683, -0.0995 };
		static const double B[8] = { 0.0056, 0.0211, 0.0262, 0.0210, 0.0189, 0.0139, 0.0085, 0.0093 };
		static const double RHO25[8] = { .1234, .2912, .3665, .3320, .3173, .2549, .1513, .2771 };
		static const double HA[8] = { 3052.6875, 14084.1349, 13874.8715, 11725.6926, 10705.4372, 9432.3599, 9878.5904, 30383.6738 };
		static const double TH[8] = { 1000., 306.3032, 306.3034, 306.2953, 306.2822, 306.2541, 306.2656, 298.1790 };
		static const double HH[8] = { 10000000., 145897.5655, 145764.3844, 144139.4767, 142970.4584, 141036.8798, 141771.3485, 38767.4058 };
		static const double TL[8] = { 290.1634, 284.3810, 284.3362, 284.0992, 284.0958, 284.1600, 284.1300, 100.0 };
		static const double HL[8] = { -31371.32, -37414.4513, -37277.0196, -36540.1186, -36502.7371, -36648.3753, -36576.3413, -100000000. };

		static const double R = 1.987;

		//because of over wintering, stage are incremented by 1
		int s = stage + 1;

		double RATE = 0;
		if (m_bLinear)
		{
			RATE = A[s] + B[s] * (TK - 273.15);
		}
		else
		{
			double ARGA = RHO25[s] * TK / 298.15;
			double ARGB = exp((HA[s] / R)*(1. / 298.15 - 1. / TK));
			double ARGC = exp((HL[s] / R)*(1. / TL[s] - 1. / TK));
			double ARGD = exp((HH[s] / R)*(1. / TH[s] - 1. / TK));
			RATE = ((ARGA*ARGB) / (1. + ARGC + ARGD));
		}

		return __max(0, RATE);
	}
	//------------------------------------------------------------
	//    This subroutine calculates the amount of physiological  
	//    development which occurs after exposure to a given      
	//    temperature for 1 hour. The total development that      
	//    occurs in a given day is then calculated. For feeding   
	//    stages, a temperature correction factor (tbud) is used  
	//    to account for diurnal deviations in bud temperature    
	//    from recorded air temperature. A bark temperature       
	//    correction factor (tbark) can be used to correct for    
	//    solar effects on overwintering larvae.                  
	//------------------------------------------------------------
	void CJackpineModel::Devel(const CDailyWaveVector& T, double& OverWinDev, double stageDev[NB_INSTAR])
	{
		//t must be hourly because TBUD and TBARK are hourly; 
		_ASSERTE(T.size() == 24);
		_ASSERTE(TIME_STEP == 1);

		//------------------------------------------------------------
		//    set time step for development as a proportion of        
		//    1 day. here the time step is .04167 days.               
		//------------------------------------------------------------
		//static const double P = .04167;
		const double P = 1.0 / T.size();

		static const double TBUD[24] = { 0, 0, 0, 0, 0, 0, 0, 0, .57, 2.61, 2.61, 2.48, 1.99, 1.31, .90, .16, .17, .21, 0, 0, 0, 0, 0, 0 };
		static const double TBARK[24] = { 0.68, 0.57, 0.57, 0.52, 0.50, 0.45, 0.45, 0.43, 0.29, 0.04, 0.80, 0.95, 1.64, 2.15, 2.38, 2.66, 2.26, 1.40, 1.02, 0.99, 1.30, 1.24, 0.71, 0.68 };

		//	static const double R=1.987;

		//-----------------------------------------
		//      zero out the counting arrays       
		//-----------------------------------------

		OverWinDev = 0;
		for (int i = L2; i < NB_INSTAR; i++)
			stageDev[i] = 0;


		//-----------------------------------------
		//   CALCULATE MICROCLIMATE TEMPERATURES   
		//-----------------------------------------
		for (int i = 0; i < (int)T.size(); i++)
		{
			const double TK1 = m_bUseBark ? (T[i] + TBARK[i] + 273.15) : (T[i] + 273.15);
			const double TK2 = m_bUseBud ? (T[i] + TBUD[i] + 273.15) : (T[i] + 273.15);

			//-----------------------------------------
			//    CALCULATE JPBW DEVELOPEMENT RATES    
			//-----------------------------------------
			OverWinDev += GetRate(TK1, OVERWINDEV)*P;

			for (int j = L2; j <= PUPEA; j++)
			{
				double rate = GetRate(TK2, j);
				stageDev[j] += rate*P;
			}
		}
	}


	//------------------------------------------------------------
	//    This subroutine calculates the probability of           
	//    individuals within an instar's age class transfering    
	//    to the next stage, and stores these probabilities in a  
	//    matrix called trans[i][j].                              
	//------------------------------------------------------------
	void CJackpineModel::ComputeTransferProb(const double age[NB_INSTAR][NB_CLASS], double transferProb[NB_INSTAR][NB_CLASS])
	{
		//-----------------------------------------------
		//    TRANSFORMATION PARAMETERS FOR EACH STAGE.  
		//-----------------------------------------------

		//FOLD is FNEW of the last day. 
		double ETA[8] = { .2899, .4282, .4048, .5212, .6742, .2536, .1439, 1000000 };
		double BETA[8] = { 1.5811, 2.9445, 2.2239, 2.1570, 1.8512, 1.5488, 1.9476, 1.0 };
		double GAMMA[8] = { .6862, .5302, .5583, .4480, .3322, .7350, .8475, 4 };


		double FNEW[NB_INSTAR][NB_CLASS] = { 0 };
		for (int i = L2; i < NB_INSTAR; i++)
		{
			for (int j = 0; j < NB_CLASS; j++)
			{
				if (age[i][j] <= GAMMA[i])
				{
					transferProb[i][j] = 0;
					FNEW[i][j] = 0;
				}
				else if (age[i][j] < 1 + 0.5*GAMMA[i])
				{
					FNEW[i][j] = 1 - exp(-pow(((age[i][j] - GAMMA[i]) / ETA[i]), BETA[i]));
					transferProb[i][j] = FNEW[i][j] - FOLD[i][j];
				}
				else
				{
					transferProb[i][j] = 1.0;
				}
			}
		}


		for (int i = L2; i < NB_INSTAR; i++)
		{
			for (int j = NB_CLASS - 1; j >= 1; j--)
				FOLD[i][j] = FNEW[i][j - 1];

			FOLD[i][0] = 0;
		}
	}

	//------------------------------------------------------------
	//   This subroutine calculates the probability of emergence  
	//   for overwintering insects.                               
	//------------------------------------------------------------
	//FOLDE is FNEWE of the last day. 
	double CJackpineModel::GetEmergeProb(double overWinAge)
	{
		static const double ETA = 0.2905;
		static const double BETA = 1.3115;
		static const double GAMMA = 0.7000;
		static const double XMAX = 1.9809;

		double FNEWE = 0;
		double emergeProb = 0;
		if (overWinAge <= GAMMA)
		{
			FNEWE = 0;
			emergeProb = 0;
		}
		else if (overWinAge < XMAX) // GAMMA < overWinAge < XMAX
		{
			FNEWE = 1 - exp(-pow((overWinAge - GAMMA) / ETA, BETA));
			emergeProb = FNEWE - FOLDE;
		}
		else
		{
			FNEWE = 1.0;
			emergeProb = FNEWE - FOLDE;
		}


		FOLDE = FNEWE;

		return emergeProb;
	}


	void CJackpineModel::Therrien(double ddays, double p[NB_INSTAR + 1], double& lysykai)
	{
		static const double insparm[8] =
		{
			189.176, //nbOverWinBugs
			359.789, //2 
			420.137, //3 
			540.139, //4 
			606.045, //5 
			680.561, //6 
			854.619, //7 
			1068.424 ///PUPAE 
		};

		static const double hostparm = 3.512;

		p[0] = 1;
		for (int i = 1; i < 9; i++)
			p[i] = 0;


		if (ddays > 0)
		{
			p[0] = 1. / (1. + exp(-((insparm[0] - ddays) / sqrt(hostparm*ddays))));
			for (int i = 1; i < 8; i++)
			{
				p[i] = 1. / (1. + exp(-((insparm[i] - ddays) / sqrt(hostparm*ddays))))
					- 1. / (1. + exp(-((insparm[i - 1] - ddays) / sqrt(hostparm*ddays))));
			}


			p[8] = 1. / (1. + exp((insparm[7] - ddays) / sqrt(hostparm*ddays)));

			lysykai = p[1] * 2 + p[2] * 3 + p[3] * 4 + p[4] * 5 + p[5] * 6 + p[6] * 7 + p[7] * 8 + p[8] * 9;
			//if( lysykai== 0 && ddays > insparm[7] )
			//lysykai=9;
		}

		if (lysykai < 2) lysykai = 2;
		if (lysykai > 9) lysykai = 9;

	}


	double CJackpineModel::Allen(double TMIN, double TMAX, double BASE)
	{
		double DD = 0;
		double MEAN = (TMIN + TMAX) / 2;

		if (TMAX < BASE)
		{
			DD = 0;
		}
		else if (TMIN < BASE)
		{
			double A = TMAX - TMIN;
			double THETA = asin(BASE - MEAN / A);
			DD = ((MEAN - BASE)*(1.570796 - THETA) + A*cos(THETA)) / 6.283181;
		}
		else
		{
			DD = MEAN - BASE;
		}

		return DD;
	}

}