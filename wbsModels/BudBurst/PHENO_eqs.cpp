#include "PHENO_eqs.h"
#include <algorithm>
#include <assert.h>
#include <boost/math/distributions/beta.hpp>


using namespace std;
namespace WBSF
{

	namespace HBB
	{


		//#define FABRIZIO_MODEL_OLD 1
		//#define FABRIZIO_MODEL_NEW 2


				//#define THE_MODEL FABRIZIO_MODEL_NEW

		static const std::array<CParameters, NB_SBW_SPECIES> PARAMETERS =
		{ {
				//"S_0","St_0","C_0","Bdw_0","v_sw","k_sw","eff_sw","v_mob","k_mob","FH_v1","FH_k1","v_acc","k_acc","v_mob2","k_mob2","FH_v2","FH_k2","v_s","k_s","k_t","S_sigma","St_max","Gtmin","Gtopt","Gtmax","I_c","FD_v1","FD_k1","FD_v2","FD_k2","FD_muS","Mtmin","Mtopt","Mtmax","B_v","B_k","B_eff","Def_mid","Def_slp","Def_min","SLA","PS_Tmin","PS_Topt","PS_Tmax","PS_PAR_1","PS_PAR_2","PS_PAR_3","a1","a2","b1","b2","dmax","bud_dw","buds_num","C_DW_Ratio","mg2g","CH2O2C","r_nb","I_max","Ftmin","Ftopt","Ftmax","S_min","S_max","S_mu","BB_thr","St_min", switch, switch
					{20,10,1000,0.125,0.37,1.6154e-03,0.716428,1.0404,247.71,1.0456,1.54962e-03,1.10784,112.522,1.48629,109.582,5.32465,103.526,0.529797,207.776,5.35064,0.000926936,99.2394,5.10764,34.7182,46.9897,14.0832,5.72638,104.64,7.36783,110.22,31.356,4.08611,16.4475,39.3847,10.1622,0.8145228,1.0500,88.3809,21.0082,0.9569,0.00555,-2,20,42,3.50,15.,-0.2413,1,0.0025,3,0.45,2.2864,-40,-5,5.10764,0,600,-5,2, 0, FABRIZIO_MODEL_OLD, false, false},
					{20,10,1000,0.125,0.37,1.6154e-03,0.716428,1.0404,247.71,1.0456,1.54962e-03,1.10784,112.522,1.48629,109.582,5.32465,103.526,0.529797,207.776,5.35064,0.000926936,99.2394,5.10764,34.7182,46.9897,14.0832,5.72638,104.64,7.36783,110.22,31.356,4.08611,16.4475,39.3847,10.1622,0.8145228,1.0500,88.3809,21.0082,0.9569,0.00555,-2,20,42,3.50,15.,-0.2413,1,0.0025,3,0.45,2.2864,-40,-5,5.10764,0,600,-5,2, 0, FABRIZIO_MODEL_OLD, false, false},
					{20,10,1000,0.125,0.28,1.7893e-03,0.661015,1.0195,220.83,1.0571,1.69442e-03,1.14047,104.911,1.02309,106.747,5.26354,108.376,0.528052,222.181,5.26675,0.000905935,92.3277,5.10218,27.3528,41.7731,29.1557,5.10557,106.077,9.7836,106.04,35.803,4.08174,14.7904,39.7261,7.28038,0.8127693,0.6025,10.2237,20.3774,0.9657,0.00443,00,16,42,12.5,3.5,-0.1254,1,0.0023,3,0.45,2.2399,-40,-5,5.10218,0,600,-5,2, 0, FABRIZIO_MODEL_OLD, false, false},
					{0},
					{0}
			} };

		void CVariables::limitToZero() { S = max(0.0, S); St = max(0.0, St); Mdw = max(0.0, Mdw); Bdw = max(0.0, Bdw); C = max(0.0, C); I = max(0.0, I); }

		void CParameters::InitBeta()
		{

			//array < double, NB_K> K = { Sw_k,B_k,G_k1,G_k2,Mob_k2,Mob_k1,Acc_k,FH_k1,FH_k2,FD_k1,FD_k2 };
			m_K = { { {Sw_k,Sw_kk},{B_k,B_kk},{G_k1,G_kk1},{G_k2,G_kk2},{Mob_k2,Mob_kk2},{Mob_k1,Mob_kk1},{Acc_k,Acc_kk},{FH_k1,FH_kk1},{FH_k2,FH_kk2},{FD_k1,FD_kk1},{FD_k2,FD_kk2}  } };

			//for (size_t t = 0; t < NB_K; t++)
			//{
			//	boost::math::beta_distribution<double> K_dist(1, abs(m_K[t]));
			//	for (size_t i = 0; i < NB_BINS + 1; i++)
			//	{
			//		double S = double(i) / NB_BINS;
			//		//m_beta_function[t][i] = cdf(K_dist, K[t]<0?(1-S):S);

			//		m_beta_function[t][i] =  (m_K[t] < 0)? 1 - S / (S + fabs(m_K[t])) : S / (S + fabs(m_K[t]));
			//	}
			//}


		}


		double CParameters::GetRatio(size_t t, double v)const
		{
			//return (m_K[t] < 0) ? 1 - max(0.0, v) / (max(0.0, v) + fabs(m_K[t])) : max(0.0, v) / (max(0.0, v) + fabs(m_K[t]));
			v = max(0.0, v);
			if (t == MOB_K2_BETA || t== FH_K2_BETA || t== FD_K2_BETA)
				return v / (m_K[t][0] + v);



			double  p = (v - m_K[t][0]) / m_K[t][1];
			return exp(-0.5 * p*p) / (m_K[t][1] * sqrt(2 * 3.1415926535897932));
		}

		CVariables PhenologyConiferEquationsOld(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx);
		CVariables PhenologyConiferEquationsNew(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx);
		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx)
		{


			CVariables out;
			switch (P.m_model)
			{
			case FABRIZIO_MODEL_OLD: out = PhenologyConiferEquationsOld(I, x, P, def, outputEx); break;
			case FABRIZIO_MODEL_NEW: out = PhenologyConiferEquationsNew(I, x, P, def, outputEx); break;
			default: _ASSERTE(false);
			}


			return  out;
		}

		CVariables PhenologyConiferEquationsNew(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx)
		{
			static const double mg2g = 0.001;
			static const double CH2O2C = 0.40641993115856;
			static const double I_MAX = 1.0;


			double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/g DW]
			double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]


			//Processes sugar(soluble carbon in shoots)
			double Ndw_0 = P.NB_r * P.Bdw_0 * (1 - def.previous);//Initial needles weight [g]
			double Ndw = P.NB_r * (x.Bdw - P.Bdw_0 + x.Mdw) * (1 - def.current) + Ndw_0;//Needles weight [g]
			
			//double PS = max(0.0, I.PN * Ndw * (1 - S_conc/ P.S_max))* (1 + 3 * def.previous);//Photosynthesis  [mg/d]
			double PS = I.PN * Ndw * (1 - S_conc / P.S_max) * (1 + 3 * def.previous);//Photosynthesis  [mg/d]
			


			// Meristems
			double Swelling_sink = P.Sw_v * I.RC_M_Tair * P.Swell_switch * !P.Budburst_switch * (1 - x.I / I_MAX);  // swelling demand
			double Swelling = Swelling_sink * P.GetRatio(SW_K_BETA, S_conc - P.S_min);
			double Prod_Mdw = Swelling * P.Sw_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C;

			// Shoot growth (both branch and needles)
			double Growth_Bdw_Ndw_sink = P.B_v * (1 - x.I / I_MAX) * I.RC_G_Tair * P.Budburst_switch * (1 + P.NB_r);   // Shoots growth demand;
			double Growth_Bdw_Ndw = Growth_Bdw_Ndw_sink * P.GetRatio(B_K_BETA, S_conc - P.S_min);   // Shoots growth;
			double Prod_Bdw = Growth_Bdw_Ndw * P.B_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C / (1 + P.NB_r); // Only branch, No needles!

			// Growth other parts of the plant(roots and stem)
			double C_SINK = max(0.0, P.G_v1 * x.C * P.GetRatio(G_K1_BETA, S_conc - P.S_min) * I.RC_G_Tsoil - Swelling - Growth_Bdw_Ndw);
			double Translocation = C_SINK + P.GetRatio(G_K2_BETA, S_conc - P.S_min) * max(0.0, PS - Swelling - Growth_Bdw_Ndw);
			double Mobilization_Stock = P.Mob_v2 * P.GetRatio(MOB_K2_BETA, x.C) * max(0.0, Growth_Bdw_Ndw + Swelling - PS);   // C Stock mobilization when[S] is very low

			//Mobilization y Accumulation 
			double Mobilization = P.Mob_v1 * P.GetRatio(MOB_K1_BETA, St_conc - P.St_min) * I.RC_M_Tair * (Swelling + Growth_Bdw_Ndw + Translocation); // Starch mobilization when[S] is very low
			double Accumulation = P.Acc_v * P.GetRatio(ACC_K_BETA, S_conc - P.S_min) * (1 - P.St_ratio(St_conc)) * I.RC_M_Tair * max(0.0, (PS - Growth_Bdw_Ndw - Swelling));


			// Frost hardening
			//double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tair - P.S_mu)));
			//Charrier 2018 use the mean maximum of the last 14 days 
			
			double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tmax14Days - P.S_mu)));
			double Frost_hardening_1 = max(0.0, P.FH_v1 * P.GetRatio(FH_K1_BETA, St_conc - P.St_min) * I.RC_F_Tair * (1 - S_conc / Smax));    // mobilization of S for frost protection
			double Frost_hardening_2 = max(0.0, P.FH_v2 * P.GetRatio(FH_K2_BETA, x.C) * I.RC_F_Tair * (1 - S_conc / Smax) - Frost_hardening_1);    // mobilization of S for frost protection
			// Frost dehardening
			double deFH_switch = 1.0 / (1.0 + exp(-(S_conc - P.FD_muS)));
			double Frost_dehardening_1 = P.FD_v1 * P.GetRatio(FD_K1_BETA, S_conc - P.S_min) * I.RC_M_Tair * (1 - P.St_ratio(St_conc)) * deFH_switch;    // mobilization of S for frost protection
			double Frost_dehardening_2 = P.FD_v2 * P.GetRatio(FD_K2_BETA, S_conc - P.S_min) * I.RC_M_Tair * deFH_switch;
			double Frost_dehardening = Frost_dehardening_1 + Frost_dehardening_2;


			//Growth inhibitor(I)
			double Prod_I = P.I_c * (Prod_Bdw + Prod_Mdw);
			double Removal_I = 0.04 * x.I * I.RC_F_Tair;

			//Equations
			_ASSERTE(PS>=0);
			_ASSERTE(Mobilization >= 0);
			_ASSERTE(Mobilization_Stock >= 0);
			_ASSERTE(Accumulation >= 0);
			_ASSERTE(Swelling >= 0);
			_ASSERTE(Growth_Bdw_Ndw >= 0);
			_ASSERTE(Translocation >= 0);
			_ASSERTE(Frost_hardening_1 >= 0);
			_ASSERTE(Frost_hardening_2 >= 0);
			_ASSERTE(Frost_dehardening_1 >= 0);
			_ASSERTE(Frost_dehardening_2 >= 0);
			_ASSERTE(Prod_Mdw >= 0);
			_ASSERTE(Prod_Bdw >= 0);
			_ASSERTE(C_SINK >= 0);
			_ASSERTE(Prod_I >= 0);
			_ASSERTE(Removal_I >= 0);
			

			double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth_Bdw_Ndw - Translocation + Frost_hardening_1 + Frost_hardening_2 - Frost_dehardening;
			double dSt = Accumulation - Mobilization - Frost_hardening_1 + Frost_dehardening_1;
			double dMdw = Prod_Mdw;
			double dBdw = Prod_Bdw;
			double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening_2 + Frost_dehardening_2;
			double dI = Prod_I - Removal_I;

			outputEx.PS = PS;
			outputEx.Mobilization = Mobilization;
			outputEx.Mobilization_Stock = Mobilization_Stock;
			outputEx.Accumulation = Accumulation;
			outputEx.Swelling = Swelling;
			outputEx.Translocation = Translocation;
			outputEx.Growth_Bdw_Ndw = Growth_Bdw_Ndw;
			outputEx.Frost_hardening1 = Frost_hardening_1;
			outputEx.Frost_hardening2 = Frost_hardening_2;
			outputEx.Frost_dehardening1 = Frost_dehardening_1;
			outputEx.Frost_dehardening2 = Frost_dehardening_2;
			outputEx.C_SINK = C_SINK;
			outputEx.Prod_I = Prod_I;
			outputEx.Removal_I = Removal_I;
			outputEx.Swell_switch = P.Swell_switch;


			//Output: variation for the current day
			return  { dS, dSt, dMdw, dBdw, dC, dI };
		}




		CVariables PhenologyConiferEquationsOld(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx)
		{
			static const double mg2g = 0.001;
			static const double CH2O2C = 0.40641993115856;
			static const double I_max = 1.0;



			double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/g DW]
			double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]


			//Processes sugar(soluble carbon in shoots)
			double Ndw_0 = P.NB_r * P.Bdw_0 * (1 - def.previous);//Needle weight[g]
			double Ndw = P.NB_r * (x.Bdw - P.Bdw_0 + x.Mdw) * (1 - def.current) + Ndw_0;//Needle weight[g]
			double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tair - P.S_mu)));
			double PS = I.PN * Ndw * (1 - S_conc / P.S_max) * (1 + 3 * def.previous);

			// Meristems
			double RC_M_Tair = P.RC_M(I.Tair);// dehardening and meristem growth
			double Swelling_sink = P.Sw_v * RC_M_Tair * P.Swell_switch * !P.Budburst_switch * (1 - x.I / I_max);  // swelling demand
			double Swelling = Swelling_sink * (S_conc / ((P.Sw_k) + S_conc));
			double Prod_Mdw = Swelling * P.Sw_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C;

			// Shoot growth (both branch and needles)
			double RC_G_Tair = P.RC_G(I.Tair);// growth
			double Growth_Bdw_Ndw_sink = P.B_v * (1 - x.I / I_max) * RC_G_Tair * P.Budburst_switch * (1 + P.NB_r);   // Shoots growth demand;
			double Growth_Bdw_Ndw = Growth_Bdw_Ndw_sink * (S_conc / ((P.B_k) + S_conc));   // Shoots growth;
			double Prod_Bdw = Growth_Bdw_Ndw * P.B_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C / (1 + P.NB_r); // Only branch, No needles!

			// Growth other parts of the plant(roots and stem)
			double RC_G_Tsoil = P.RC_G(I.Tsoil);// growth
			double C_SINK = max(0.0, P.G_v1 * x.C * (S_conc / (P.G_k1 + S_conc)) * RC_G_Tsoil - Swelling - Growth_Bdw_Ndw);
			double Translocation = C_SINK + (S_conc / (P.G_k2 + S_conc)) * max(0.0, PS - Swelling - Growth_Bdw_Ndw);
			double Mobilization_Stock = P.Mob_v2 * (x.C / (P.Mob_k2 + x.C)) * max(0.0, Growth_Bdw_Ndw + Swelling - PS);   // C Stock mobilization when[S] is very low

			//Mobilization y Accumulation 
			double Mobilization = P.Mob_v1 * (St_conc / ((P.Mob_k1) + St_conc)) * RC_M_Tair * (Swelling + Growth_Bdw_Ndw + Translocation); // Starch mobilization when[S] is very low
			double Accumulation = P.Acc_v * (S_conc / ((P.Acc_k) + S_conc)) * (1 - (St_conc - P.St_min) / (P.St_max - P.St_min)) * RC_M_Tair * max(0.0, (PS - Growth_Bdw_Ndw - Swelling));


			// Frost hardening
			double RC_F_Tair = P.RC_F(I.Tair);
			double Frost_hardening1 = max(0.0, P.FH_v1 * (St_conc / ((P.FH_k1) + St_conc)) * RC_F_Tair * (1 - S_conc / Smax));    // mobilization of S for frost protection
			double Frost_hardening2 = max(0.0, P.FH_v2 * (x.C / ((P.FH_k2) + x.C)) * RC_F_Tair * (1 - S_conc / Smax) - Frost_hardening1);    // mobilization of S for frost protection
			double deFH_switch = 1.0 / (1.0 + exp(-1.0 * (S_conc - P.FD_muS)));
			double Frost_dehardening_1 = (P.FD_v1 * (S_conc / ((P.FD_k1) + S_conc)) * RC_M_Tair * (1 - (St_conc - P.St_min) / (P.St_max - P.St_min))) * deFH_switch;    // mobilization of S for frost protection
			double Frost_dehardening_2 = (P.FD_v2 * (S_conc / ((P.FD_k2) + S_conc)) * RC_M_Tair) * deFH_switch;
			double Frost_dehardening = Frost_dehardening_1 + Frost_dehardening_2;


			//Growth inhibitor(I)
			double Prod_I = P.I_c * (Prod_Bdw + Prod_Mdw);
			double Removal_I = 0.04 * x.I * RC_F_Tair;

			//Equations
			double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth_Bdw_Ndw - Translocation + Frost_hardening1 + Frost_hardening2 - Frost_dehardening;
			double dSt = Accumulation - Mobilization - Frost_hardening1 + Frost_dehardening_1;
			double dMdw = Prod_Mdw;
			double dBdw = Prod_Bdw;
			double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening2 + Frost_dehardening_2;
			double dI = Prod_I - Removal_I;

			outputEx.PS = PS;
			outputEx.Mobilization = Mobilization;
			outputEx.Mobilization_Stock = Mobilization_Stock;
			outputEx.Accumulation = Accumulation;
			outputEx.Swelling = Swelling;
			outputEx.Translocation = Translocation;
			outputEx.Growth_Bdw_Ndw = Growth_Bdw_Ndw;
			outputEx.Frost_hardening1 = Frost_hardening1;
			outputEx.Frost_hardening2 = Frost_hardening2;
			outputEx.Frost_dehardening1 = Frost_dehardening_1;
			outputEx.Frost_dehardening2 = Frost_dehardening_2;
			outputEx.C_SINK = C_SINK;
			outputEx.Prod_I = Prod_I;
			outputEx.Removal_I = Removal_I;
			outputEx.Swell_switch = P.Swell_switch;


			//Output: variation for the current day
			return  { dS, dSt, dMdw, dBdw, dC, dI };
		}

		//#elif THE_MODEL==FABRIZIO_MODEL_NEW
		//
		//		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx)
		//		{
		//			static const double mg2g = 0.001;
		//			static const double CH2O2C = 0.40641993115856;
		//			static const double I_MAX = 1.0;
		//
		//
		//
		//			double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/g DW]
		//			double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]
		//
		//
		//			//Processes sugar(soluble carbon in shoots)
		//			double Ndw_0 = P.NB_r * P.Bdw_0 * (1 - def.previous);//Needle weight[g]
		//			double Ndw = P.NB_r * (x.Bdw - P.Bdw_0 + x.Mdw) * (1 - def.current) + Ndw_0;//Needle weight[g]
		//			double PS = I.PN * Ndw * (1 - S_conc / P.S_max) * (1 + 3 * def.previous);
		//
		//			// Meristems growth
		//			double Swelling_sink = P.Sw_v * I.RC_M_Tair * P.Swell_switch * !P.Budburst_switch * (1 - x.I / I_MAX);  // swelling demand
		//			double Swelling = Swelling_sink * (S_conc / (P.Sw_k + S_conc));
		//			double Prod_Mdw = Swelling * P.Sw_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C;
		//
		//			// Shoot growth (both branch and needles)
		//			double Growth_Bdw_Ndw_sink = P.B_v * (1 - x.I / I_MAX) * I.RC_G_Tair * P.Budburst_switch * (1 + P.NB_r);   // Shoots growth demand;
		//			double Growth_Bdw_Ndw = Growth_Bdw_Ndw_sink * (S_conc / (P.B_k + S_conc));   // Shoots growth;
		//			double Prod_Bdw = Growth_Bdw_Ndw * P.B_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C / (1 + P.NB_r); // Only branch, No needles!
		//
		//			// Growth other parts of the plant(roots and stem)
		//			double C_SINK = max(0.0, P.G_v1 * x.C * (S_conc / (P.G_k1 + S_conc)) * I.RC_G_Tsoil - Swelling - Growth_Bdw_Ndw);
		//			double Translocation = C_SINK + (S_conc / (P.G_k2 + S_conc)) * max(0.0, PS - Swelling - Growth_Bdw_Ndw);
		//			double Mobilization_Stock = P.Mob_v2 * (x.C / (P.Mob_k2 + x.C)) * max(0.0, Growth_Bdw_Ndw + Swelling - PS);   // C Stock mobilization when[S] is very low
		//
		//			//Mobilization y Accumulation 
		//			double Mobilization = P.Mob_v1 * (St_conc / (P.Mob_k1 + St_conc)) * I.RC_M_Tair * (Swelling + Growth_Bdw_Ndw + Translocation); // Starch mobilization when[S] is very low
		//			double Accumulation = P.Acc_v * (S_conc / (P.Acc_k + S_conc)) * (1 - (St_conc - P.St_min) / (P.St_max - P.St_min)) * I.RC_M_Tair * max(0.0, (PS - Growth_Bdw_Ndw - Swelling));
		//
		//
		//			// Frost hardening
		//			//double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tair - P.S_mu)));
		//			//Charrier 2018 use the mean maximum of the last 14 days 
		//			double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tmax14Days - P.S_mu)));
		//			double Frost_hardening_1 = max(0.0, P.FH_v1 * (St_conc / (P.FH_k1 + St_conc)) * I.RC_F_Tair * (1 - S_conc / Smax));    // mobilization of S for frost protection
		//			double Frost_hardening_2 = max(0.0, P.FH_v2 * (x.C / (P.FH_k2 + x.C)) * I.RC_F_Tair * (1 - S_conc / Smax) - Frost_hardening_1);    // mobilization of S for frost protection
		//			// Frost dehardening
		//			double deFH_switch = 1.0 / (1.0 + exp(-1.0 * (S_conc - P.FD_muS)));
		//			double Frost_dehardening_1 = (P.FD_v1 * (S_conc / (P.FD_k1 + S_conc)) * I.RC_M_Tair * (1 - (St_conc - P.St_min) / (P.St_max - P.St_min))) * deFH_switch;    // mobilization of S for frost protection
		//			double Frost_dehardening_2 = (P.FD_v2 * (S_conc / (P.FD_k2 + S_conc)) * I.RC_M_Tair) * deFH_switch;
		//			double Frost_dehardening = Frost_dehardening_1 + Frost_dehardening_2;
		//
		//
		//			//Growth inhibitor(I)
		//			double Prod_I = P.I_c * (Prod_Bdw + Prod_Mdw);
		//			double Removal_I = 0.04 * x.I * I.RC_F_Tair;
		//
		//			//Equations
		//			double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth_Bdw_Ndw - Translocation + Frost_hardening_1 + Frost_hardening_2 - Frost_dehardening;
		//			double dSt = Accumulation - Mobilization - Frost_hardening_1 + Frost_dehardening_1;
		//			double dMdw = Prod_Mdw;
		//			double dBdw = Prod_Bdw;
		//			double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening_2 + Frost_dehardening_2;
		//			double dI = Prod_I - Removal_I;
		//
		//			outputEx.PS = PS;
		//			outputEx.Mobilization = Mobilization;
		//			outputEx.Mobilization_Stock = Mobilization_Stock;
		//			outputEx.Accumulation = Accumulation;
		//			outputEx.Swelling = Swelling;
		//			outputEx.Translocation = Translocation;
		//			outputEx.Growth_Bdw_Ndw = Growth_Bdw_Ndw;
		//			outputEx.Frost_hardening1 = Frost_hardening_1;
		//			outputEx.Frost_hardening2 = Frost_hardening_2;
		//			outputEx.Frost_dehardening1 = Frost_dehardening_1;
		//			outputEx.Frost_dehardening2 = Frost_dehardening_2;
		//			outputEx.C_SINK = C_SINK;
		//			outputEx.Prod_I = Prod_I;
		//			outputEx.Removal_I = Removal_I;
		//			outputEx.Swell_switch = P.Swell_switch;
		//
		//
		//			//Output: variation for the current day
		//			return  { dS, dSt, dMdw, dBdw, dC, dI };
		//		}
		//



	}
}






