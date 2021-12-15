#pragma once

#include <math.h>
#include <array>
#include <cmath>


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
			return (T > Tmin && T < Tmax) ? (2 * pow(T - Tmin, alpha) * pow(Topt - Tmin, alpha) - pow(T - Tmin, 2 * alpha)) / pow(Topt - Tmin, 2 * alpha) : 0;
		}

		enum TModel { FABRIZIO_MODEL_OLD, FABRIZIO_MODEL_NEW, NB_MODELS };


		//static double RC2(double T, double Tmin, double Topt, double Tmax)
		//{
		//	double x = (Tmax - T) / (Tmax - Topt);
		//	double y = (T - Tmin) / (Topt - Tmin);
		//	double z = (Topt - Tmin) / (Tmax - Topt);

		//	return (T > Tmin && T < Tmax) ? x* pow(y,z): 0;
		//}

		enum TBeta { SW_K_BETA, B_K_BETA, G_K1_BETA, G_K2_BETA, MOB_K2_BETA, MOB_K1_BETA, ACC_K_BETA, FH_K1_BETA, FH_K2_BETA, FD_K1_BETA, FD_K2_BETA, NB_K, NB_BINS = 1000 };
		class CParameters
		{
		public:

			double S_0;
			double St_0;
			double C_0;
			double Bdw_0;
			double Sw_v;
			double Sw_k;
			double Sw_eff;
			double Mob_v1;
			double Mob_k1;
			double FH_v1;
			double FH_k1;
			double Acc_v;
			double Acc_k;
			double Mob_v2;
			double Mob_k2;
			double FH_v2;
			double FH_k2;
			double G_v1;
			double G_k1;
			double G_k2;
			double S_sigma;
			double St_max;
			double G_minT;
			double G_optT;
			double G_maxT;
			double I_c;
			double FD_v1;
			double FD_k1;
			double FD_v2;
			double FD_k2;
			double FD_muS;
			double M_minT;
			double M_optT;
			double M_maxT;
			double B_v;
			double B_k;
			double B_eff;
			double Def_mid;
			double Def_slp;
			double Def_min;
			double PAR_SLA;
			double PAR_minT;
			double PAR_optT;
			double PAR_maxT;
			double PAR_PS1;
			double PAR_PS2;
			double PAR_PS3;
			//double a1;
			//double a2;
			//double b1;
			//double b2;
			double Def_max;
			double bud_dw;
			double buds_num;
			double C_DW_Ratio;
			//double mg2g;
			//double CH2O2C;
			double NB_r;
			//double I_max;
			double F_minT;
			double F_optT;
			double F_maxT;
			double S_min;
			double S_max;
			//			double BBphase;
			double S_mu;
			//double v_c;
			double BB_thr;
			double St_min;
			//double Sw_thres;
			TModel m_model;

			//double Sw_kk;
			//double B_kk;
			//double G_kk1;
			//double G_kk2;
			//double Mob_kk2;
			//double Mob_kk1;
			//double Acc_kk;
			//double FH_kk1;
			//double FH_kk2;
			//double FD_kk1;
			//double FD_kk2;

			bool Swell_switch;
			bool Budburst_switch;
			double G_v2;
			double C_min;
			double C_max;
			size_t m_nbSteps;

			double RC_G(double T)const { return RC(T, G_minT, G_optT, G_maxT); }
			double RC_F(double T)const { return RC(T, F_minT, F_optT, F_maxT); }
			double RC_M(double T)const { return RC(T, M_minT, M_optT, M_maxT); }
			double RC_PAR(double T)const { return RC(T, PAR_minT, PAR_optT, PAR_maxT); }


			//double S_ratio(double S_conc) const { return std::max(0.001, std::min(0.999, (S_conc - S_min) / (S_max - S_min))); }
			//double St_ratio(double St_conc) const { return std::max(0.001, std::min(0.999, (St_conc - St_min) / (St_max - St_min))); }
			//double C_ratio(double C) const { return std::max(0.001, std::min(0.999, C / 3000)); }
			void InitBeta();
			size_t value_to_bin(double v)const { return std::max(size_t(0), std::min(size_t(NB_BINS), size_t(std::round(v * NB_BINS)))); }
			double GetBeta(size_t t, double v)const { return m_beta_function[t][value_to_bin(v)]; }
			double GetRatio(size_t t, double v, double v_min, double v_max)const;


			bool is_valid()const
			{

				if (G_minT > G_optT || G_optT > G_maxT)
					return false;

				if (F_minT > F_optT || F_optT > F_maxT)
					return false;

				if (M_minT > M_optT || M_optT > M_maxT)
					return false;

				if (PAR_minT > PAR_optT || PAR_optT > PAR_maxT)
					return false;

				if (C_min > C_max)
					return false;

				/*if (m_model == FABRIZIO_MODEL_NEW)
				{
					if ((m_P[CU_σ_PS] > -0.1 && m_P[CU_σ_PS] < 0.1) ||
						(m_P[FU_σ_PS] > -0.1 && m_P[FU_σ_PS] < 0.1))
						return false;
				}*/


				return true;
			}

			//std::array < std::array < double, 2>, NB_K> m_K;
			std::array < double, NB_K> m_K;
			std::array< std::array < double, NB_BINS + 1>, NB_K> m_beta_function;

		};


		class COldParam
		{
		public:

			double G_minT;
			double G_optT;
			double G_maxT;

			double F_minT;
			double F_optT;
			double F_maxT;

			double M_minT;
			double M_optT;
			double M_maxT;

			double PAR_minT;
			double PAR_optT;
			double PAR_maxT;

			double PAR_PS1;
			double PAR_PS2;
			double PAR_PS3;

			COldParam& operator =(const CParameters& in)
			{
				G_minT = in.G_minT;
				G_optT = in.G_optT;
				G_maxT = in.G_maxT;
				F_minT = in.F_minT;
				F_optT = in.F_optT;
				F_maxT = in.F_maxT;
				M_minT = in.M_minT;
				M_optT = in.M_optT;
				M_maxT = in.M_maxT;
				PAR_minT = in.PAR_minT;
				PAR_optT = in.PAR_optT;
				PAR_maxT = in.PAR_maxT;
				PAR_PS1 = in.PAR_PS1;
				PAR_PS2 = in.PAR_PS2;
				PAR_PS3 = in.PAR_PS3;

				return *this;
			}


			bool e(double e1, double e2)const { return fabs(e1 - e2) < 0.05; }
			bool operator ==(const CParameters& in)const
			{
				return
					e(G_minT, in.G_minT) &&
					e(G_optT, in.G_optT) &&
					e(G_maxT, in.G_maxT) &&
					e(F_minT, in.F_minT) &&
					e(F_optT, in.F_optT) &&
					e(F_maxT, in.F_maxT) &&
					e(M_minT, in.M_minT) &&
					e(M_optT, in.M_optT) &&
					e(M_maxT, in.M_maxT) &&
					e(PAR_minT, in.PAR_minT) &&
					e(PAR_optT, in.PAR_optT) &&
					e(PAR_maxT, in.PAR_maxT) &&
					e(PAR_PS1, in.PAR_PS1) &&
					e(PAR_PS2, in.PAR_PS2) &&
					e(PAR_PS3, in.PAR_PS3);
			}


			bool operator !=(const CParameters& in)const { return !operator ==(in); }

			

		};


		extern const std::array<CParameters, NB_SBW_SPECIES> PARAMETERS;

		class CVariables
		{
		public:

			CVariables operator+(const CVariables& in)const { return { S + in.S,St + in.St,Mdw + in.Mdw,Bdw + in.Bdw,C + in.C,I + in.I }; }
			CVariables operator/(double f)const { return { S / f, St / f,Mdw / f,Bdw / f,C / f,I / f }; }
			void limitToZero();



			double S;//Stem Sugar [mg]
			double St;//Stem Starch[mg]
			double Mdw;//Meristem (buds?) dry weigh [g]
			double Bdw;//Branch (Stem) dry weigh [g]
			double C;//Carbohydrates? [mg]
			double I;//Inhibitor factor [0..1]

		};

		class COutputEx
		{
		public:

			double PS;
			double Mobilization;
			double Mobilization_Stock;
			double Accumulation;
			double Swelling;
			double Translocation;
			double Growth_Bdw_Ndw;
			double Frost_hardening1;
			double Frost_hardening2;
			double Frost_dehardening1;
			double Frost_dehardening2;
			double C_SINK;
			double Prod_I;
			double Removal_I;
			double Swell_switch;
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

			CInput operator+(const CInput& in) { return { Tair + in.Tair,Tsoil + in.Tsoil, PN + in.PN }; }
			CInput operator*(double f) { return { Tair * f,Tsoil * f,PN * f }; }
			double Tair;
			double Tmax14Days;
			double Tsoil;
			double PN;
			double RC_G_Tair;
			double RC_G_Tsoil;
			double RC_F_Tair;
			double RC_M_Tair;


		};



		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx);

	}
}