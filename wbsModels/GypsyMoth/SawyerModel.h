#pragma once

#include "EggModel.h"

namespace WBSF
{


	class CSawyerModel : public CEggModel
	{
	public:

		CSawyerModel(const CGMEggParam& param);
		~CSawyerModel();

		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p);

	private:

		static const double lower_thresh[3];
		static const double upper_thresh[3];
		static const double psiI;
		static const double rhoI;
		static const double TmI;
		static const double deltaTI;
		static const double alphaI;
		static const double betaI;
		static const double gammaI;

		static const double Tl0[2];
		static const double Th0[2];
		static const double Topt0[2];
		static const double Ropt0[2];

		static const double Tl1[2];
		static const double Th1[2];
		static const double Topt1[2];
		static const double Ropt1[2];
		static const double g[2][5];


		void initialize_prediapause_ages(void);
		void calc_prediapause_variability(void);
		void calc_prediapause_rates();
		void init_prediapause();
		double calc_prediap_rate(double T);
		void allocate_prediapause_array(void);

		double dellvf(double rin, double *r, double *rates, double *delp, double dt, int k);
		void getRates(double T, double *Tl, double *Th, double *Topt, double *Ropt, double *Rt);

		enum { NCLASS1 = 1 };
		double *prediap_table; //dynamically allocated, must be freed
		double prediap_age[NCLASS1];
		double prediap_variability[NCLASS1];


	};

}