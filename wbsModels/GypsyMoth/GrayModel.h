#pragma once

#include "EggModel.h"

namespace WBSF
{

	typedef CModelStat CEggState;


	//Gray's model parameters. Need resetting if not read from set file
	class CGrayModel : public CEggModel
	{
	public:

		static const int N_CLASS1 = 10;        // These values can be changed at will (max:20) 
		static const int N_CLASS2 = 5;       // but 5 5 10 seems quite enough 
		static const int N_CLASS3 = 10;
		static const int MAXCLASSES = 20;      // to dimension arrays 
		static const int AGERANGEPOSTDIAPAUSE = 21;     //0.05 increments from 0 to 1.0, 

		CGrayModel(const CGMEggParam& param);


		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p);

	private:

		void Init();
		void ComputeTotalEgg(CTRef day, const CWeatherStation& weather, CEggState& tot_eggs);
		void ComputeHatching();


		static const double lower_thresh[3];     //  lower temperature thresholds of phases
		static const double upper_thresh[3];   //  upper temperature thresholds of phases

		static const double psiI, rhoI, TmI, deltaTI;                 // prediapause developmental rate parameters 
		static const double PDR_c, PDR_t, PDR_t2, PDR_t4;               // diapause PDR parameters
		static const double start_inhibitor, RS_c, RS_rp, RP_c;         // diapause inhibitor depletion parameters
		static const double eff_res_1, eff_res_2;                     // effective resistance parameters for the inhibitor
		static const double tauIII, deltaIII;                        // postdiapause developmental rate parameters
		static const double omegaIII, kappaIII, psiIII, thetaIII;    //  parameters of phaseIII rate change
		static const double alphaI, betaI, gammaI;                   // variability parameters for phase I
		static const double alphaII, betaII, gammaII;                // variability parameters for phase II
		static const double alphaIII, betaIII, gammaIII;             // variability parameters for phase III

		void calc_rates();
		void calc_variability(void);
		double prediapause_rate(double T);
		double depletion_rate(double T, double inhib_titre);
		double diapause_rate(double T, double inhib_titre);
		double postdiapause_rate(double T, double age);
		void allocate_arrays(int days);
		void initialize_ages(void);
		void GrayReset(int days);
		void free_Gray_arrays();



		//****************************
		double *prediapause_table;   //Lookup table of prediapause rate for each temperature
		double **d_inhibitor_table;  //Lookup table of inhibitor depletion rate for each temperature
		double **diapause_table;     //Lookup table of diapause rate for each temperature & inhibitor titre combination
		double **postdiapause_table; //Lookup table of postdiapause rate for each temperature & inhibitor titre combination
		double variability_table[4][MAXCLASSES];    //10 rate classes for each phase plus hatch
		double prediapause_age[N_CLASS1];         // age of each class in prediapause
		double inhibitor_titre[N_CLASS1][N_CLASS2]; //inhibitor level of each class in diapause
		double diapause_age[N_CLASS1][N_CLASS2];         //  age of each class in diapause
		double postdiapause_age[N_CLASS1][N_CLASS2][N_CLASS3];  //age of each class in postdiapause
		double diapause_eggs[N_CLASS1];
		double postdiapause_eggs[N_CLASS1][N_CLASS2];

		static const double class_size[3];
		static const int number_classes[3];    //number of variability classes in 3 phases

		//Rounds off double to integers
		static int round_off(double input)
		{
			if (input < 0)return((int)(input - 0.5));
			else if (input>0)return((int)(input + 0.5));
			else return(0);
		}


		int age_range_postdiapause;       //re-initialized in GrayReset

	};

}
