#include "PHENO_eqs.h"
#include <algorithm>
#include <assert.h>
#include <boost/math/distributions/beta.hpp>


using namespace std;
namespace WBSF
{

	namespace HBB
	{


		static const std::array < std::array<CParameters, NB_SBW_SPECIES>, NB_VERSIONS>  PARAMETERS =
		{ {
			//S_conc0,St_conc0,C_0,B_0,v_S,k_S,eff_S,v_M,k_M,v_F1,k_F1,v_A,k_A,v_M2,k_M2,v_F2,k_F2,v_C,k_C,k_T,k_slp,cSt_max,Gtmin,Gtopt,Gtmax,c,v_FO1,k_FO1,v_FO2,k_FO2,cS_low,Mtmin,Mtopt,Mtmax,v_B,k_B,eff_B,mid,slope,d_min,SLA,PS_Tmin,PS_Topt,PS_Tmax,p1,p2,p3,bud_dw,buds_num,C_DW_Ratio,r_NB,Ftmin,Ftopt,Ftmax,cS_min,cS_max,BB_thr,SDI_mu,SDI_sigma
			
			{{//Original
				{20,0,1000,0.125,0.370198,1.78e-05,0.716428,1.04042,247.71,1.0456,0.00155,1.10784,112.5222,1.4862855,120.814155,5.5908825,103.526,0.55628685,207.776,5.35064,0.000926936,114.882010425,5.10764,34.7182,46.9897,14.0832,10.2837557482066,104.64,7.36783,115.731,31.356,4.08611,16.4475,39.3847,10.1622,0.8145228,1.0500970725,88.3809,21.0082,1,0.0056,-2,20,42,3.5,15,-0.24131907862159,0.0025,3,0.45,2.28642133771412,-40,-5,5.6311731,0,200,2,5.8284,0.8331},
				{20,0,1000,0.125,0.370198,1.78e-05,0.716428,1.04042,247.71,1.0456,0.00155,1.10784,112.5222,1.4862855,120.814155,5.5908825,103.526,0.55628685,207.776,5.35064,0.000926936,114.882010425,5.10764,34.7182,46.9897,14.0832,10.2837557482066,104.64,7.36783,115.731,31.356,4.08611,16.4475,39.3847,10.1622,0.8145228,1.0500970725,88.3809,21.0082,1,0.0056,-2,20,42,3.5,15,-0.24131907862159,0.00037,3,0.45,2.28642133771412,-40,-5,5.6311731,0,200,2,5.8284,0.8331},
				{20,0,1000,0.125,0.28,1.88e-05,0.661015,1.0195,231.87885,1.05707,0.00169,1.14047,104.911,1.0742445,106.747,5.26354,108.376,0.528052,222.18105,5.26675,0.000905935,101.79128925,5.10218,27.3528,41.7731,30.613485,5.10557,106.077,10.2728325,111.3483,35.8038,4.08174,14.7904,41.712405,7.28038,0.8127693,0.6025,10.2237,20.3774,1,0.0044,0,16,42,12.5,3.5,-0.125379039151047,0.0023,3,0.45,2.23990845349878,-40,-5,5.10218,0,200,2,5.6553,0.7802},
				{20,0,1000,0.125,0.370198,1.78e-05,0.716428,1.04042,247.71,1.0456,0.00155,1.10784,112.5222,1.4862855,120.814155,5.5908825,103.526,0.55628685,207.776,5.35064,0.000926936,114.882010425,5.10764,34.7182,46.9897,14.0832,10.2837557482066,104.64,7.36783,115.731,31.356,4.08611,16.4475,39.3847,10.1622,0.8145228,1.0500970725,88.3809,21.0082,1,0.0056,-2,20,42,3.5,15,-0.24131907862159,0.0025,3,0.45,2.28642133771412,-40,-5,5.6311731,0,200,2,5.8284,0.8331},
			}},
			{{//recalibrate
				{20,0,1000,0.125,0.1442,56.2718,4.8343,15.0342,76.296,9.4675,108.2043,13.2065,20.5037,1.486,109.58,7.6564,149.6249,0.5298,207.76,5.351,10.85,199.9931,5.1,41.6855,47,14.083,27.5307,149.7551,2.7206,35.0465,21.0667,4.1,20.6567,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,40.3981,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,-9.7034,5.1,33.1806,94.7049,2,4.0744,0.4017},
				{20,0,1000,0.125,0.3054,0.0648,1.1767,1.2442,66.9672,34.3646,244.5086,20.7875,145.8163,1.486,109.58,17.6189,107.1381,0.5298,207.76,5.351,1.0717,148.4002,5.1,33.2614,47,14.083,8.7425,75.0041,13.3813,57.8802,25.1237,4.1,22.865,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,38.538,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,1.3101,5.1,47.5358,75.1808,2,3.8597,0.6444},
				{20,0,1000,0.125,0.1635,13.8833,2.2142,4.153,42.6234,20.6811,49.9193,6.0098,155.6005,0.6636,139.8699,16.4616,89.5295,14.8646,116.6601,236.2704,13.2611,121.3585,5.10218,37.3342,41.7731,29.1557,30.3601,190.9422,13.9524,189.6555,24.3191,4.08174,20.8812,39.7261,35.0273,1.9114,12.6342,10.2237,20.3774,1,0.0044,0,40.0199,42,12.5,3.5,-0.1254,0.0023,3,0.45,2.23990845349878,-40,-5.5419,5.3657,4.4442,77.7923,2,4.2971,0.7094},
				{20,0,1000,0.125,1.2703,159.7976,4.0499,22.4673,94.3026,3.291,248.5034,1.5432,145.5832,1.486,109.58,16.1374,137.5349,0.5298,207.76,5.351,2.3312,146.5889,5.1,42.8712,47,14.083,62.4572,213.135,13.5578,51.5146,23.4445,4.1,28.863,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,40.3743,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,-4.03,5.1,0.9357,77.8838,2,4.1178,0.3194 },
			}},
			{{//Modified
				{20,0,1000,0.125,0.1442,56.2718,4.8343,15.0342,76.296,9.4675,108.2043,13.2065,20.5037,1.486,109.58,7.6564,149.6249,0.5298,207.76,5.351,10.85,199.9931,5.1,41.6855,47,14.083,27.5307,149.7551,2.7206,35.0465,21.0667,4.1,20.6567,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,40.3981,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,-9.7034,5.1,33.1806,94.7049,2,4.0744,0.4017},
				{20,0,1000,0.125,0.3054,0.0648,1.1767,1.2442,66.9672,34.3646,244.5086,20.7875,145.8163,1.486,109.58,17.6189,107.1381,0.5298,207.76,5.351,1.0717,148.4002,5.1,33.2614,47,14.083,8.7425,75.0041,13.3813,57.8802,25.1237,4.1,22.865,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,38.538,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,1.3101,5.1,47.5358,75.1808,2,3.8597,0.6444},
				{20,0,1000,0.125,0.1635,13.8833,2.2142,4.153,42.6234,20.6811,49.9193,6.0098,155.6005,0.6636,139.8699,16.4616,89.5295,14.8646,116.6601,236.2704,13.2611,121.3585,5.10218,37.3342,41.7731,29.1557,30.3601,190.9422,13.9524,189.6555,24.3191,4.08174,20.8812,39.7261,35.0273,1.9114,12.6342,10.2237,20.3774,1,0.0044,0,40.0199,42,12.5,3.5,-0.1254,0.0023,3,0.45,2.23990845349878,-40,-5.5419,5.3657,4.4442,77.7923,2,4.2971,0.7094},
				{20,0,1000,0.125,1.2703,159.7976,4.0499,22.4673,94.3026,3.291,248.5034,1.5432,145.5832,1.486,109.58,16.1374,137.5349,0.5298,207.76,5.351,2.3312,146.5889,5.1,42.8712,47,14.083,62.4572,213.135,13.5578,51.5146,23.4445,4.1,28.863,39.4,10.16,0.8127693,1.05,88.3809,21.0082,1,0.005555,-2,40.3743,42,3.5,15,-0.24132,0.0025,3,0.45,2.28642133771412,-40,-4.03,5.1,0.9357,77.8838,2,4.1178,0.3194 },
			}}
		 } };






		void CVariables::limitToZero() { S = max(0.0, S); St = max(0.0, St); Mdw = max(0.0, Mdw); Bdw = max(0.0, Bdw); C = max(0.0, C); I = max(0.0, I); }

		void CParameters::InitBeta()
		{

			m_K = { Sw_k,B_k,G_k1,G_k2,Mob_k2,Mob_k1,Acc_k,FH_k1,FH_k2,FD_k1,FD_k2 };

			for (size_t t = 0; t < NB_K; t++)
			{
				boost::math::beta_distribution<double> K_dist(1, max(0.001, abs(m_K[t])));
				for (size_t i = 0; i < NB_BINS + 1; i++)
				{
					double S = double(i) / NB_BINS;
					m_beta_function[t][i] = cdf(K_dist, m_K[t] < 0 ? (1 - S) : S);
					//	m_beta_function[t][i] = (m_K[t] < 0) ? 1 - S / (S + fabs(m_K[t])) : S / (S + fabs(m_K[t]));
				}
			}
		}


		double CParameters::GetRatio(size_t t, double v, double v_min, double v_max)const
		{
			size_t bin = value_to_bin((v - v_min) / (v_max - v_min));
			return m_beta_function[t][bin];
		}



		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def, COutputEx& outputEx, bool bEx)
		{
			static const double mg2g = 0.001;
			static const double CH2O2C = 0.40641993115856;
			static const double I_max = 1.0;
			static const double R_I = 0.04;


			double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/g DW]
			double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]


			//Processes sugar(soluble carbon in shoots)
			double Ndw_0 = P.NB_r * P.Bdw_0 * (1 - def.previous);//Needle weight[g]
			double Ndw = P.NB_r * (x.Bdw - P.Bdw_0 + x.Mdw) * (1 - def.current) + Ndw_0;//Needle weight[g]

			double PS = I.PN * Ndw * (1 - S_conc / P.cS_max) * (1 + 3 * def.previous);


			// Meristems
			double RC_M_Tair = P.RC_M(I.Tair);// dehardening and meristem growth
			double Swelling = P.Sw_v * (S_conc / ((P.Sw_k) + S_conc)) * RC_M_Tair * x.Swell_switch * !x.Budburst_switch * (1 - x.I / I_max);


			// Shoot growth (both branch and needles)
			double RC_G_Tair = P.RC_G(I.Tair);// growth
			double Growth = P.B_v * (S_conc / ((P.B_k) + S_conc)) * RC_G_Tair * x.Budburst_switch * (1 + P.NB_r) * (1 - x.I / I_max);   // Shoots growth;


			// Growth other parts of the plant(roots and stem)

			double RC_G_Tsoil = P.RC_G(I.Tsoil);// growth
			double C_SINK = max(0.0, P.G_v1 * x.C * (S_conc / (P.G_k1 + S_conc)) * RC_G_Tsoil - Swelling - Growth);
			double Translocation = C_SINK + (S_conc / (P.G_k2 + S_conc)) * max(0.0, PS - Swelling - Growth);
			double Mobilization_Stock = P.Mob_v2 * (x.C / (P.Mob_k2 + x.C)) * max(0.0, Growth + Swelling - PS);   // C Stock mobilization when[S] is very low

			//Mobilization y Accumulation 
			double Mobilization = P.Mob_v1 * (St_conc / ((P.Mob_k1) + St_conc)) * RC_M_Tair * (Swelling + Growth + Translocation); // Starch mobilization when[S] is very low
			double Accumulation = P.Acc_v * (S_conc / ((P.Acc_k) + S_conc)) * (1 - St_conc / P.St_max) * RC_M_Tair * max(0.0, (PS - Growth - Swelling));


			// Frost hardening
			//double S_FH = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.S_sigma * (I.Tair - P.S_mu)));
			double S_FH = P.cS_max + (P.cS_min - P.cS_max) / (1 + exp(-P.FH_sigma * (I.Tair - P.F_optT)));
			double RC_F_Tair = P.RC_F(I.Tair);
			double Frost_hardening1 = max(0.0, P.FH_v1 * (St_conc / ((P.FH_k1) + St_conc)) * RC_F_Tair * (1 - S_conc / S_FH));    // mobilization of S for frost protection
			double Frost_hardening2 = max(0.0, P.FH_v2 * (x.C / ((P.FH_k2) + x.C)) * RC_F_Tair * (1 - S_conc / S_FH) - Frost_hardening1);    // mobilization of S for frost protection
			
			double deFH_switch = 1.0 / (1.0 + exp(-1.0 * (S_conc - P.FD_muS)));
			double Frost_dehardening1 = (P.FD_v1 * (S_conc / ((P.FD_k1) + S_conc)) * RC_M_Tair * (1 - St_conc / P.St_max)) * deFH_switch;    // mobilization of S for frost protection
			double Frost_dehardening2 = (P.FD_v2 * (S_conc / ((P.FD_k2) + S_conc)) * RC_M_Tair) * deFH_switch;
			//double Frost_dehardening = Frost_dehardening_1 + Frost_dehardening_2;


			//Growth inhibitor(I)
			double Prod_Mdw = Swelling * P.Sw_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C;
			double Prod_Bdw = Growth * P.B_eff * (1 / P.C_DW_Ratio) * mg2g * CH2O2C / (1 + P.NB_r); // Only branch, No needles!
			double Prod_I = P.I_c * (Prod_Bdw + Prod_Mdw);
			double Removal_I = R_I * x.I * RC_F_Tair;

			//Equations
			double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth - Translocation + Frost_hardening1 + Frost_hardening2 - Frost_dehardening1 - Frost_dehardening2;
			double dSt = Accumulation - Mobilization - Frost_hardening1 + Frost_dehardening1;
			double dMdw = Prod_Mdw;
			double dBdw = Prod_Bdw;
			double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening2 + Frost_dehardening2;
			double dI = Prod_I - Removal_I;

			if (bEx)
			{
				outputEx.PS = PS;
				outputEx.Mobilization = Mobilization;
				outputEx.Mobilization_Stock = Mobilization_Stock;
				outputEx.Accumulation = Accumulation;
				outputEx.Swelling = Swelling;
				outputEx.Translocation = Translocation;
				outputEx.Growth_Bdw_Ndw = Growth;
				outputEx.Frost_hardening1 = Frost_hardening1;
				outputEx.Frost_hardening2 = Frost_hardening2;
				outputEx.Frost_dehardening1 = Frost_dehardening1;
				outputEx.Frost_dehardening2 = Frost_dehardening2;
				outputEx.C_SINK = C_SINK;
				outputEx.Prod_I = Prod_I;
				outputEx.Removal_I = Removal_I;
				outputEx.Swell_switch = x.Swell_switch;
			}

			//Output: variation for the current day
			return  { dS, dSt, dMdw, dBdw, dC, dI };
		}


	}
}






