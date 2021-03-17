#include "PHENO_eqs.h"
#include <algorithm>

using namespace std;
namespace WBSF
{

	namespace HBB
	{
		 

		static const CParameters PARAMETERS[NB_SBW_SPECIES] =
		{
			//"S_0","St_0","C_0","Bdw_0","v_sw","k_sw","eff_sw","v_mob","k_mob","v_FH","k_FH","v_acc","k_acc","v_mob2","k_mob2","v_FH2","k_FH2","v_s","k_s","k_t","k_slp","St_max","Gtmin","Gtopt","Gtmax","c","v_FO1","k_FO1","v_FO2","k_FO2","S_low","Mtmin","Mtopt","Mtmax","v_b","k_b","eff_b","mid","slope","dmin","SLA","PS_Tmin","PS_Topt","PS_Tmax","PS_PAR_1","PS_PAR_2","PS_PAR_3","a1","a2","b1","b2","dmax","bud_dw","buds_num","C_DW_Ratio","mg2g","CH2O2C","r_nb","I_max","Ftmin","Ftopt","Ftmax","S_min","S_max","BBphase","T_mid","v_c","BB_thr", switch, switch
				{20,10,1000,0.125,0.370198,1.6154e-05,0.716428,1.04042,2.4771,1.0456,1.54962e-05,1.10784,1.125222,1.4862855,1.09582,5.32465,1.03526,0.529797,2.07776,0.0535064,0.000926936,99.2394,5.10764,34.7182,46.9897,14.0832,5.72638,1.0464,7.36783,1.1022,31.356,4.08611,16.4475,39.3847,10.1622,0.008145228,1.0500970725,88.3809,21.0082,0.956886,0.00555555555555556,-2,20,42,3.5,15,-0.24131907862159,0.1061,0.0168,0.038,0.0204,1,0.0025,3,0.45,0.001,0.40641993115856,2.28642133771412,1,-40,-5,5.10764,0,600,6,-5,0,2, false, false},
				{0},
				{20,10,1000,0.125,0.28,1.789302375e-05,0.661015,1.0195,2.20837,1.05707,1.69442e-05,1.14047,1.04911,1.02309,1.06747,5.26354,1.08376,0.528052,2.2218105,0.0526675,0.000905935,92.3277,5.10218,27.3528,41.7731,29.1557,5.10557,1.06077,9.78365,1.06046,35.8038,4.08174,14.7904,39.7261,7.28038,0.008127693,0.6025,10.2237,20.3774,0.965699,0.00443,0,16,42,12.5,3.5,-0.125379039151047,0.0445,0.0216,0.0139,0.0279,1,0.00231481481481481,3,0.45,0.001,0.40641993115856,2.23990845349878,1,-40,-5,5.10218,0,600,6,-5,0,2, false, false},
				{0},
				{0}
		};


		

		//CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def)
		//{
		//	
		//	//Needle weight [g]
		//	double Ndw_0 = P.Ndw_0(def.previous);
		//	double Ndw = P.r_nb * (x.Bdw - P.Bdw_0 + x.Mdw)*(1 - def.current) + Ndw_0;

		//	double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/gDW]
		//	double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/gDW]

		//	//Environmental responses
		//	double RC_G_Tair = P.RC_G(I.Tair);// growth
		//	double RC_G_Tsoil = P.RC_G(I.Tsoil);// growth
		//	double RC_F_Tair = P.RC_F(I.Tair);// Frost hardening
		//	double RC_M_Tair = P.RC_M(I.Tair);// dehardening and meristem growth

		//	

		//	//bool Veg_switch = temp_Veg || Veg_switch;           // Vegetative growth switch
		//	double deFH_switch = 1.0 / (1.0 + exp(-1.0 * (S_conc - P.S_low)));
		//	
		//	//bool Swell_switch = temp_Swell || Swell_switch;           // Vegetative growth switch

		//	//Processes sugar(soluble carbon in shoots)
		//	double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.k_slp * (I.Tair - P.T_mid)));
		//	double PS = I.PN * Ndw*(1 - S_conc / P.S_max)*(1 + 3 * def.previous);

		//	// Meristems
		//	double Swelling_sink = P.v_sw*RC_M_Tair*P.Swell_switch*(1 - P.Veg_switch)*(1 - x.I / P.I_max);  // swelling demand
		//	double Swelling = Swelling_sink * (S_conc / ((P.k_sw * 100) + S_conc));
		//	double Prod_Mdw = Swelling * P.eff_sw*(1 / P.C_DW_Ratio)*P.mg2g*P.CH2O2C;

		//	// Shoot growth (both branch and needles)
		//	double Growth_Bdw_sink = P.v_b * (1 - x.I / P.I_max)*RC_G_Tair*P.Veg_switch*(1 + P.r_nb);   // Shoots growth demand;
		//	double Growth_Bdw = Growth_Bdw_sink * (S_conc / ((P.k_b * 100) + S_conc));   // Shoots growth;
		//	double Prod_Bdw = Growth_Bdw * P.eff_b*(1 / P.C_DW_Ratio)*P.mg2g*P.CH2O2C / (1 + P.r_nb); // ONLY BRANCH, NO NEEDLES!

		//	// Growth other parts of the plant(roots and stem)
		//	double C_SINK = max(0.0, P.v_s*x.C*(S_conc / (P.k_s * 100 + S_conc))*RC_G_Tsoil - Swelling - Growth_Bdw);
		//	double Translocation = C_SINK + (S_conc / (P.k_t * 100 + S_conc))*max(0.0, PS - Swelling - Growth_Bdw);

		//	//double Mobilization_Stock = P.v_mob2 * (x.C / (P.k_mob2 * 100 + x.C))*max(0.0, Growth_Bdw_sink + Swelling_sink - PS);   // C Stock mobilization when[S] is very low
		//	double Mobilization_Stock = P.v_mob2 * (x.C / (P.k_mob2 * 100 + x.C))*max(0.0, Growth_Bdw + Swelling - PS);   // C Stock mobilization when[S] is very low

		//	double Frost_hardening1 = max(0.0, P.v_FH*(St_conc / ((P.k_FH * 100) + St_conc))*RC_F_Tair*(1 - S_conc / Smax));    // mobilization of S for frost protection
		//	double Frost_hardening2 = max(0.0, P.v_FH2*(x.C / ((P.k_FH2 * 100) + x.C))*RC_F_Tair*(1 - S_conc / Smax) - Frost_hardening1);    // mobilization of S for frost protection

		//	double Frost_hardening_1out = (P.v_FO1*(S_conc / ((P.k_FO1 * 100) + S_conc))*RC_M_Tair*(1 - St_conc / P.St_max))*deFH_switch;    // mobilization of S for frost protection
		//	double Frost_hardening_2out = (P.v_FO2*(S_conc / ((P.k_FO2 * 100) + S_conc))*RC_M_Tair)*deFH_switch;
		//	double Frost_hardening_out = Frost_hardening_1out + Frost_hardening_2out;

		//	//Mobilization y Accumulation 
		//	double Mobilization = P.v_mob * (St_conc / ((P.k_mob * 100) + St_conc))*RC_M_Tair*(Swelling + Growth_Bdw + Translocation); // Starch mobilization when[S] is very low
		//	double Accumulation = P.v_acc * (S_conc / ((P.k_acc * 100) + S_conc))*(1 - St_conc / P.St_max)*RC_M_Tair*max(0.0, (PS - Growth_Bdw - Swelling));

		//	//Growth inhibitor(I)
		//	double Prod_I = P.c * (Prod_Bdw + Prod_Mdw);
		//	double Removal_I = 0.04*x.I*(RC_F_Tair);

		//	//Equations
		//	double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth_Bdw - Translocation + Frost_hardening1 + Frost_hardening2 - Frost_hardening_out;
		//	double dSt = Accumulation - Mobilization - Frost_hardening1 + Frost_hardening_1out;
		//	double dMdw = Prod_Mdw;
		//	double dBdw = Prod_Bdw;
		//	double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening2 + Frost_hardening_2out;
		//	double dI = Prod_I - Removal_I;

		//	//Output: variation for the current day
		////	CVariables dx =
		//	return  { dS, dSt, dMdw, dBdw, dC, dI };
		//}

		

		CVariables PhenologyConiferEquations(const CInput& I, const CVariables& x, const CParameters& P, const CDefoliation& def)
		{
			
			//Needle weight[g]
			//Ndw = P$r_nb * (x$Bdw - P$Bdw_0 + x$Mdw)*(1 - def$current) + P$Ndw_0;
			//P$Budburst_thr = P$BB_thr * Mdw_0;
			//P$Ndw_0 = P$r_nb * P$Bdw_0*(1 - def$previous);
			double Ndw_0 = P.r_nb * P.Bdw_0*(1 - def.previous);
			double Ndw = P.r_nb * (x.Bdw - P.Bdw_0 + x.Mdw)*(1 - def.current) + Ndw_0;

			//S_conc = x$S / (x$Mdw + x$Bdw);       # Sugars concentration[mg / gDW]
			//St_conc = x$St / (x$Mdw + x$Bdw);     # Starch concentration[mg / gDW]

			double S_conc = x.S / (x.Mdw + x.Bdw);       // Sugars concentration [mg/gDW]
			double St_conc = x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/gDW]

			//Environmental responses
			double RC_G_Tair = P.RC_G(I.Tair);// growth
			double RC_G_Tsoil = P.RC_G(I.Tsoil);// growth
			double RC_F_Tair = P.RC_F(I.Tair);// Frost hardening
			double RC_M_Tair = P.RC_M(I.Tair);// dehardening and meristem growth

			

			//bool Veg_switch = temp_Veg || Veg_switch;           // Vegetative growth switch
			double deFH_switch = 1.0 / (1.0 + exp(-1.0 * (S_conc - P.S_low)));
			
			//Processes sugar(soluble carbon in shoots)
			double Smax = P.S_max + (P.S_min - P.S_max) / (1 + exp(-P.k_slp * (I.Tair - P.T_mid)));
			double PS = I.PN * Ndw*(1 - S_conc / P.S_max)*(1 + 3 * def.previous);

			// Meristems
			double Swelling_sink = P.v_sw*RC_M_Tair*P.Swell_switch*!P.Veg_switch*(1 - x.I / P.I_max);  // swelling demand
			double Swelling = Swelling_sink * (S_conc / ((P.k_sw * 100) + S_conc));
			double Prod_Mdw = Swelling * P.eff_sw*(1 / P.C_DW_Ratio)*P.mg2g*P.CH2O2C;

			// Shoot growth (both branch and needles)
			double Growth_Bdw_sink = P.v_b * (1 - x.I / P.I_max)*RC_G_Tair*P.Veg_switch*(1 + P.r_nb);   // Shoots growth demand;
			double Growth_Bdw = Growth_Bdw_sink * (S_conc / ((P.k_b * 100) + S_conc));   // Shoots growth;
			double Prod_Bdw = Growth_Bdw * P.eff_b*(1 / P.C_DW_Ratio)*P.mg2g*P.CH2O2C / (1 + P.r_nb); // ONLY BRANCH, NO NEEDLES!

			// Growth other parts of the plant(roots and stem)
			double C_SINK = max(0.0, P.v_s*x.C*(S_conc / (P.k_s * 100 + S_conc))*RC_G_Tsoil - Swelling - Growth_Bdw);
			double Translocation = C_SINK + (S_conc / (P.k_t * 100 + S_conc))*max(0.0, PS - Swelling - Growth_Bdw);
			double Mobilization_Stock = P.v_mob2 * (x.C / (P.k_mob2 * 100 + x.C))*max(0.0, Growth_Bdw + Swelling - PS);   // C Stock mobilization when[S] is very low

			double Frost_hardening1 = max(0.0, P.v_FH*(St_conc / ((P.k_FH * 100) + St_conc))*RC_F_Tair*(1 - S_conc / Smax));    // mobilization of S for frost protection
			double Frost_hardening2 = max(0.0, P.v_FH2*(x.C / ((P.k_FH2 * 100) + x.C))*RC_F_Tair*(1 - S_conc / Smax) - Frost_hardening1);    // mobilization of S for frost protection
			double Frost_hardening_1out = (P.v_FO1*(S_conc / ((P.k_FO1 * 100) + S_conc))*RC_M_Tair*(1 - St_conc / P.St_max))*deFH_switch;    // mobilization of S for frost protection
			double Frost_hardening_2out = (P.v_FO2*(S_conc / ((P.k_FO2 * 100) + S_conc))*RC_M_Tair)*deFH_switch;
			double Frost_hardening_out = Frost_hardening_1out + Frost_hardening_2out;

			//Mobilization y Accumulation 
			double Mobilization = P.v_mob * (St_conc / ((P.k_mob * 100) + St_conc))*RC_M_Tair*(Swelling + Growth_Bdw + Translocation); // Starch mobilization when[S] is very low
			double Accumulation = P.v_acc * (S_conc / ((P.k_acc * 100) + S_conc))*(1 - St_conc / P.St_max)*RC_M_Tair*max(0.0, (PS - Growth_Bdw - Swelling));

			//Growth inhibitor(I)
			double Prod_I = P.c * (Prod_Bdw + Prod_Mdw);
			double Removal_I = 0.04*x.I*(RC_F_Tair);

			//Equations
			double dS = PS + Mobilization + Mobilization_Stock - Accumulation - Swelling - Growth_Bdw - Translocation + Frost_hardening1 + Frost_hardening2 - Frost_hardening_out;
			double dSt = Accumulation - Mobilization - Frost_hardening1 + Frost_hardening_1out;
			double dMdw = Prod_Mdw;
			double dBdw = Prod_Bdw;
			double dC = Translocation - C_SINK - Mobilization_Stock - Frost_hardening2 + Frost_hardening_2out;
			double dI = Prod_I - Removal_I;

			//Output: variation for the current day
		//	CVariables dx =
			return  { dS, dSt, dMdw, dBdw, dC, dI };
		}

		void CVariables::limitToZero() { S = max(0.0, S); St = max(0.0, St); Mdw = max(0.0, Mdw); Bdw = max(0.0, Bdw); C = max(0.0, C); I = max(0.0, I); }

	}
}