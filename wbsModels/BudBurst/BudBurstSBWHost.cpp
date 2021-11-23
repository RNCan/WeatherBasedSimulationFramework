//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstSBWHost.h"
#include "PHENO_eqs.h"
#include <deque>
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::HBB;

namespace WBSF
{

	static const size_t MAX_SDI_STAGE = 6;
	static const size_t NB_STEPS = 5;
	
	


	CSBWHostBudBurst::CSBWHostBudBurst()
	{
		m_SDI = { 2.1616,1.8268,4.0,50 ,-6 ,26 ,10.7,0.7 ,167 ,9.47 };
	}

	CSBWHostBudBurst::~CSBWHostBudBurst()
	{
	}

	//This method is call to compute solution

	ERMsg CSBWHostBudBurst::Execute(CWeatherStation& weather, CModelStatVector& output, bool bModelEx)
	{
		ASSERT(m_SDI_type < NB_SDI_TYPE);
		ASSERT(weather.IsHourly());
		ASSERT(weather.GetNbYears() >= 2);
		ASSERT(m_species < NB_SBW_SPECIES);
		//ASSERT(m_P.mg2g == 0.001);//parameters are initialized

		ERMsg msg;

		boost::math::weibull_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);
		//boost::math::beta_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);


		CParameters P = m_P;
		P.InitBeta();

		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		output.Init(pp, bModelEx? NB_HBB_OUTPUTS_EX:NB_HBB_OUTPUTS, -999);

		


		//compute input
		// Re sample Daily Climate
		deque<CInput> input(pp.size());
		for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
				{
					CStatistic PN_stat;
					CStatistic Tair_stat;
					CStatistic RC_G_stat;
					CStatistic RC_F_stat;
					CStatistic RC_M_stat;

					for (size_t h = 0; h < 24; h++)
					{
						const CHourlyData& hData = weather[y][m][d][h];
						 
						//Calculate unitary ps
						//convert total solar radiation (SRAD) [Watt/m²] into Photo Active Radiation (PAR) μmol*m²/s
						double PAR = hData[H_SRAD] * 2.1;//Photo Active Radiation (PAR) μmol*m²/s
						//The approximation 1 Watt/m² ≈ 4.57 μmol.m2/s comes from the Plant Growth Chamber Handbook(chapter 1, radiation; https://www.controlledenvironments.org/wp-content/uploads/sites/6/2017/06/Ch01.pdf). 
						//Note that the value of 4.57 converts Watts/m2 to μmol.m2/s, assuming that the Watt/m² is for radiation from 400 to 700 nm. 
						//However, I don't know that anyone ever measures solar radiation in Watt/m² in the 400-700 nm range. 
						//Pyranometers measure total solar radiation. 
						//Since only about 45% of the energy of solar radiation is actually in the 400-700 nm range. 
						//the conversion of total solar radiation to PAR is ~2.1, rather than 4.57. 
						//As mentioned by others, that is an approximation, but a pretty good one.

						double RC_PAR_PS = P.PAR_PS1 * (1 - pow(1 - P.PAR_PS3 / P.PAR_PS2, 1 - PAR / P.PAR_PS2)) + abs(P.PAR_PS3);
						double RC_PAR_T = P.RC_PAR(hData[H_TAIR]);

						static const double umol2mgC = 0.029526111;
						double PN = max(0.0, RC_PAR_PS * RC_PAR_T * 3600 * P.PAR_SLA);    // umolCO2 gDW - 1 h - 1
						PN *= umol2mgC; // mgSugar gDW - 1 d - 1
						PN_stat += PN;

						Tair_stat += hData[H_TAIR];
						RC_G_stat += P.RC_G(hData[H_TAIR]);
						RC_F_stat += P.RC_F(hData[H_TAIR]);
						RC_M_stat += P.RC_M(hData[H_TAIR]);
					}

					input[dd].Tair = Tair_stat[MEAN];
					input[dd].PN = PN_stat[SUM];
					input[dd].RC_G_Tair = RC_G_stat[MEAN];
					input[dd].RC_F_Tair = RC_F_stat[MEAN];
					input[dd].RC_M_Tair = RC_M_stat[MEAN];

					if (dd > 14)
					{
						CStatistic Tmax14Days_stat;
						CTRef TRef = weather[y][m][d].GetTRef();
						for (size_t i = 0; i < 14; i++)//Charrier 2018 use the mean maximum of the last 14 days 
						{
							Tmax14Days_stat += weather.GetDay(TRef - i)[H_TMAX];
						}

						input[dd].Tmax14Days = Tmax14Days_stat[MEAN];
					}
				}
			}
		}


		// Estimate soil temperature from air temperature
		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			int year = weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
			// 
			// in the original code, it was from August to September
			//CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));


			CStatistic Tair = weather.GetStat(H_TAIR, p);

			double media = Tair[MEAN];
			double ampiezza = 8;
			double shift = -30;

			size_t dd = p.Begin() - pp.Begin();
			for (size_t d = 0; d < p.size(); d++, dd++)
			{
				input[dd].Tsoil = media + ampiezza * cos(PI * (d + 1 + shift) / 180);
				input[dd].RC_G_Tsoil = P.RC_G(input[dd].Tsoil);
			}
		}


		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			// Setup simulation
			int year = weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
			// in the original code, it was from August to September
			//CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));

			CDefoliation def;
			def.previous = m_defioliation[year - 1];
			def.current = m_defioliation[year];


			// Calculate current year Bud removal percentage
			def.def = P.Def_min + (P.Def_max - P.Def_min) / (1 + pow((def.current * 100) / P.Def_mid, P.Def_slp));


			// Calculate intermediate variables
			double Mdw_0 = P.bud_dw * P.buds_num*def.def;
			double Ndw_0 = P.NB_r * P.Bdw_0*(1 - def.previous);
			static const double I_0 = 1.0;
			double Budburst_thr = P.BB_thr*Mdw_0;
			P.Budburst_switch = false;
			P.Swell_switch = false;


			// State variables initial values[s st mdw bdw C I]
			CVariables x0 = {
					P.S_0 * (P.Bdw_0 + Mdw_0),
					P.St_0 * (P.Bdw_0 + Mdw_0),
					Mdw_0,
					P.Bdw_0,
					P.C_0,
					I_0
			};

			
			

			CVariables x = x0;
			size_t d = p.Begin() - pp.Begin();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++, d++)
			{

				double PS = max(0.0, min(1.0, (x.Mdw - Mdw_0) / (Budburst_thr - Mdw_0)));
				double SDI_Dhont = cdf(SDI_dist, PS) * MAX_SDI_STAGE;//0 .. 6;
				double SDI_Auger = max(0.0, min(5.0, exp(log(5) * (SDI_Dhont - 2.5) / (5.6 - 2.5)) - 0.33));//0 .. 5;

				

				output[TRef][O_S_CONC] = x.S / (x.Mdw + x.Bdw);//Sugars concentration [mg/g DW]
				output[TRef][O_ST_CONC] = x.St / (x.Mdw + x.Bdw);// Starch concentration [mg/g DW]
				output[TRef][O_MERISTEMS] = x.Mdw;//[g]
				output[TRef][O_BRANCH] = x.Bdw + x.Mdw;//[g]
				output[TRef][O_NEEDLE] = P.NB_r * (x.Bdw + x.Mdw - P.Bdw_0)*(1 - def.previous) + Ndw_0;  //[g];
				output[TRef][O_C] = x.C;
				output[TRef][O_INHIBITOR] = x.I; 
				output[TRef][O_BUDBURST] = PS *100;//[%]
				output[TRef][O_SDI] = m_SDI_type == SDI_DHONT ? SDI_Dhont : SDI_Auger;
				output[TRef][O_SUGAR] = x.S;//[mg]
				output[TRef][O_STARCH] = x.St;//[mg] 

				
				COutputEx outputEx;
				

				//size_t nbSteps = 10;
				for (size_t t = 0; t < NB_STEPS; t++)
				{
					//Phenological switches
					P.Budburst_switch |= (x.Mdw >= Budburst_thr);
					//P.Swell_switch |= x.S / (x.Mdw + x.Bdw) >= P.Sw_thres;
					P.Swell_switch = TRef.GetYear() == p.End().GetYear();//swelling only at the second year

					//double f = 1.0 - (t - 1.0) / (nbSteps - 1.0);
					//CInput I = input[d] * f + input[d + 1] * (1 - f);
					CVariables dx = PhenologyConiferEquations(input[d], x, P, def, outputEx);
					x = x + dx / NB_STEPS;

					//limit concentration to valid values
					//double S_conc = max(P.S_min, min(P.S_max, x.S / (x.Mdw + x.Bdw)));       // Sugars concentration [mg/g DW]
					//double St_conc = max(P.St_min, min(P.St_max, x.St / (x.Mdw + x.Bdw)));     // Starch concentration [mg/g DW]
					////update sugar and starch
					//x.S = S_conc * (x.Mdw + x.Bdw);
					//x.St = St_conc * (x.Mdw + x.Bdw);
					//x.S = max(1.0, x.S);
					//x.St = max(1.0, x.St);

					x.limitToZero();
				}

				

				
				if (bModelEx)
				{
					output[TRef][O_PS] = outputEx.PS;
					output[TRef][O_MOBILIZATION] = outputEx.Mobilization;
					output[TRef][O_MOBILIZATION_STOCK] = outputEx.Mobilization_Stock;
					output[TRef][O_ACCUMULATION] = outputEx.Accumulation;
					output[TRef][O_SWELLING] = outputEx.Swelling;
					output[TRef][O_TRANSLOCATION] = outputEx.Translocation;
					output[TRef][O_GROWTH_BDW_NDW] = outputEx.Growth_Bdw_Ndw;
					output[TRef][O_FROST_HARDENING1] = outputEx.Frost_hardening1;
					output[TRef][O_FROST_HARDENING2] = outputEx.Frost_hardening2;
					output[TRef][O_FROST_DEHARDENING1] = outputEx.Frost_dehardening1;
					output[TRef][O_FROST_DEHARDENING2] = outputEx.Frost_dehardening2;
					output[TRef][O_C_SINK] = outputEx.C_SINK;
					output[TRef][O_PROD_I] = outputEx.Prod_I;
					output[TRef][O_REMOVAL_I] = outputEx.Removal_I;
					output[TRef][O_SWELL_SWITCH] = P.Swell_switch;
					output[TRef][O_TAIR] = input[d].Tair;
					output[TRef][O_TSOIL] = input[d].Tsoil;
					output[TRef][O_PN] = input[d].PN;
					output[TRef][O_RC_G_TAIR] = input[d].RC_G_Tair;
					output[TRef][O_RC_F_TAIR] = input[d].RC_F_Tair;
					output[TRef][O_RC_M_TAIR] = input[d].RC_M_Tair;
					output[TRef][O_RC_G_TSOIL] = input[d].RC_G_Tsoil;
				}
			//output[TRef][O_BUDBURST] = min(100.0, round((x.Mdw - Mdw_0)*10000) / round((Budburst_thr - Mdw_0) * 10000) * 100);//[%]

				
			}

			//update S_0 and St_0
			P.S_0 = x0.S;
			P.St_0 = x0.St;

		}


		return msg;
	}


}