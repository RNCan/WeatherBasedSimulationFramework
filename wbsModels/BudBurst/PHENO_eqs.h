#pragma once

#include <math.h>


namespace WBSF
{
	//SBW host Budburst
	namespace HBB
	{
		enum TSBWSpecies { BALSAM_FIR, WHITE_SPRUCE, BLACK_SPRUCE, NORWAY_SPUCE, RED_SPRUCE, NB_SBW_SPECIES };


		static double Alpha(double Tmin, double Topt, double Tmax) { return log(2.0) / log((Tmax - Tmin) / (Topt - Tmin)); }
		static double RC(double T, double Tmin, double Topt, double Tmax)
		{
			double alpha = Alpha(Tmin, Topt, Tmax);
			return (T > Tmin && T < Tmax) ? (2 * pow(T - Tmin, alpha)*pow(Topt - Tmin, alpha) - pow(T - Tmin, 2 * alpha)) / pow(Topt - Tmin, 2 * alpha) : 0;
		}


		class CParameters
		{
		public:

			double S_0;
			double St_0;
			double C_0;
			double Bdw_0;
			double v_sw;
			double k_sw;
			double eff_sw;
			double v_mob;
			double k_mob;
			double v_FH;
			double k_FH;
			double v_acc;
			double k_acc;
			double v_mob2;
			double k_mob2;
			double v_FH2;
			double k_FH2;
			double v_s;
			double k_s;
			double k_t;
			double k_slp;
			double St_max;
			double Gtmin;
			double Gtopt;
			double Gtmax;
			double c;
			double v_FO1;
			double k_FO1;
			double v_FO2;
			double k_FO2;
			double S_low;
			double Mtmin;
			double Mtopt;
			double Mtmax;
			double v_b;
			double k_b;
			double eff_b;
			double mid;
			double slope;
			double dmin;
			double SLA;
			double PS_Tmin;
			double PS_Topt;
			double PS_Tmax;
			double PS_PAR_1;
			double PS_PAR_2;
			double PS_PAR_3;
			double a1;
			double a2;
			double b1;
			double b2;
			double dmax;
			double bud_dw;
			double buds_num;
			double C_DW_Ratio;
			double mg2g;
			double CH2O2C;
			double r_nb;
			double I_max;
			double Ftmin;
			double Ftopt;
			double Ftmax;
			double S_min;
			double S_max;
			double BBphase;
			double T_mid;
			double v_c;
			double BB_thr;
			bool Swell_switch;
			bool Veg_switch;



			//double Def(double def)const { return dmin + (dmax - dmin) / pow(1 + (def * 100 / mid), slope); }
			//double Mdw_0(double def)const { return bud_dw * buds_num*Def(def); }
			//double Ndw_0(double def)const { return r_nb * Bdw_0*(1 - def); }

			//double Budburst_thr(double def)const { return BB_thr * Mdw_0(def); }

			double RC_G(double T)const { return RC(T, Gtmin, Gtopt, Gtmax); }
			double RC_F(double T)const { return RC(T, Ftmin, Ftopt, Ftmax); }
			double RC_M(double T)const { return RC(T, Mtmin, Mtopt, Mtmax); }
			double RC_PS(double T)const { return RC(T, PS_Tmin, PS_Topt, PS_Tmax); }


		};

		extern const CParameters PARAMETERS[NB_SBW_SPECIES];

		class CVariables
		{
		public:

			CVariables operator+(const CVariables& in)const { return { S + in.S,St + in.St,Mdw + in.Mdw,Bdw + in.Bdw,C + in.C,I + in.I }; }
			CVariables operator/(double f)const { return { S / f, St / f,Mdw / f,Bdw / f,C / f,I / f }; }
			void limitToZero() ;
				


			double S;
			double St;
			double Mdw;
			double Bdw;
			double C;
			double I;

		};




		class CDefoliation
		{
		public:

			double previous;
			double current;
			double def;
		};

		class CInput
		{
		public:
			
			CInput operator+(const CInput& in) { return { Tair+in.Tair,Tsoil+in.Tsoil, PN+in.PN }; }
			CInput operator*(double f) { return { Tair*f,Tsoil*f,PN*f}; }
			double Tair;
			double Tsoil;
			double PN;
		};



		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def);

	}
}