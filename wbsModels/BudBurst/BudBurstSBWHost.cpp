//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation from Fabrizio Carteni MathLab code, see article : https://nph.onlinelibrary.wiley.com/doi/full/10.1111/nph.18974
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

	static const size_t MAX_SDI_STAGE = 5;


	CSBWHostBudBurst::CSBWHostBudBurst()
	{
		m_nbSteps = 1;
		m_bCumul = false;
	}

	CSBWHostBudBurst::~CSBWHostBudBurst()
	{
	}


	double CSBWHostBudBurst::Weibull(size_t stage, double  SDI, const array < double, 2>& p, size_t first_stage, size_t last_stage)
	{
		//scale, shape
		enum TParam { lambda, k };

		double Fx = -999;
		if (stage < last_stage)
			Fx = 1.0 - exp(-pow(p[lambda] * (SDI - first_stage), p[k]));
		else if (stage == last_stage)
			Fx = exp(-pow(p[lambda] * (last_stage - SDI), p[k]));

		return Fx;
	}

	array<double, 6> CSBWHostBudBurst::SDI_2_Sx(double SDI, bool bCumul)
	{
		static const array< array< double, 2>, 5> P =
		{ {
			{1.561, 1.501},//F1
			{0.552, 3.022},//F2
			{0.372, 4.365},//F3
			{0.273, 6.994},//F4
			{1.444, 1.390},//F5
		} };


		double F1 = Weibull(1, SDI, P[0]);
		double F2 = Weibull(2, SDI, P[1]);
		double F3 = Weibull(3, SDI, P[2]);
		double F4 = Weibull(4, SDI, P[3]);
		double F5 = Weibull(5, SDI, P[4]);

		array<double, 6> Sx = { 0 };

		if (bCumul)
			Sx = { 1 - F1, F1, F2, F3, F4, F5 };
		else
			Sx = { 1 - F1, F1 - F2, F2 - F3, F3 - F4, F4 - F5, F5 };

		return Sx;
	}


	double CSBWHostBudBurst::Weight2Length(size_t s, double w, TW2L type)
	{
		ASSERT(type < NB_W2L);

		double l = 0;
		if (type == W2L_LORENA)
		{
			static const double P[4][2] =
			{
				{0.03823901, 0.02170693},//bf
				{0.01003552, 0.04105784},//ws
				{0.01106281, 0.03137205},//bs
				{0.01003552, 0.04105784},//ns same as ws!
			};

			l = log(max(1.0, w / P[s][1])) / P[s][2];
		}
		else if (type == W2L_REMI)
		{
			static const double P[4][3] =
			{
				{0.08548114, 0.01620569, 0.07649309}, //bf
				{0.02345253, 0.03187149, 0.02497838}, //ws
				{0.04148479, 0.02063676, 0.05425486}, //bs
				{0.02345253, 0.03187149, 0.02497838}  //ns same as ws!
			};

			l = log(max(1.0, (w + P[s][3]) / P[s][1])) / P[s][2];
		}


		return l;
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

		boost::math::weibull_distribution<double> SDI_dist(m_P.SDI_mu, m_P.SDI_sigma);
		//boost::math::beta_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);


		//CParameters P = m_P;
		//P.InitBeta();

		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		output.Init(pp, bModelEx ? NB_HBB_OUTPUTS_EX : NB_HBB_OUTPUTS, -999);




		//compute input
		// Re sample Daily Climate

		if (m_mean_T_day.empty() || m_P_last != m_P)
		{


			m_P_last = m_P;
			m_mean_T_day.resize(pp.GetNbDay());

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

							//double RC_PAR_PS = P.PAR_PS1 * (1 - pow(1 - P.PAR_PS3 / P.PAR_PS2, 1 - PAR / P.PAR_PS2)) + abs(P.PAR_PS3);
							double RC_PAR_PS = m_P.PAR_PS1 * (1 - pow(1 - m_P.PAR_PS3 / m_P.PAR_PS1, 1 - PAR / m_P.PAR_PS2)) + abs(m_P.PAR_PS3);
							double RC_PAR_T = m_P.RC_PAR(hData[H_TAIR]);

							static const double umol2mgC = 0.029526111;
							double PN = RC_PAR_PS * RC_PAR_T * 3600 * m_P.PAR_SLA;    // umolCO2 gDW - 1 h - 1
							PN *= umol2mgC; // mgSugar gDW - 1 d - 1
							PN_stat += PN;

							Tair_stat += hData[H_TAIR];



							RC_G_stat += m_P.RC_G(hData[H_TAIR]);
							RC_F_stat += m_P.RC_F(hData[H_TAIR]);
							RC_M_stat += m_P.RC_M(hData[H_TAIR]);
						}

						m_mean_T_day[dd].Tair = Tair_stat[MEAN];
						m_mean_T_day[dd].PN = PN_stat[SUM];

						if (m_version == V_ORIGINAL || m_version == V_RECALIBRATED)
						{
							m_mean_T_day[dd].RC_G_Tair = m_P.RC_G(m_mean_T_day[dd].Tair);
							m_mean_T_day[dd].RC_F_Tair = m_P.RC_F(m_mean_T_day[dd].Tair);
							m_mean_T_day[dd].RC_M_Tair = m_P.RC_M(m_mean_T_day[dd].Tair);
						}
						else
						{
							m_mean_T_day[dd].RC_G_Tair = RC_G_stat[MEAN];
							m_mean_T_day[dd].RC_F_Tair = RC_F_stat[MEAN];
							m_mean_T_day[dd].RC_M_Tair = RC_M_stat[MEAN];
						}

						//if (dd > 14)
						//{
						//	CStatistic Tmax14Days_stat;
						//	CTRef TRef = weather[y][m][d].GetTRef();
						//	for (size_t i = 0; i < 14; i++)//Charrier 2018 use the mean maximum of the last 14 days 
						//	{
						//		Tmax14Days_stat += weather.GetDay(TRef - i)[H_TMAX];
						//	}

						//	m_mean_T_day[dd].Tmax14Days = Tmax14Days_stat[MEAN];
						//}
					}
				}
			}


			// Estimate soil temperature from air temperature
			for (size_t y = 1; y < weather.GetNbYears(); y++)
			{
				int year = weather[y].GetTRef().GetYear();
				CTPeriod p;
				if (m_version == V_ORIGINAL || m_version == V_RECALIBRATED)
					p = CTPeriod(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
				else if (m_version == V_MODIFIED)
					//p = CTPeriod(CTRef(year - 1, SEPTEMBER, DAY_01), CTRef(year, AUGUST, DAY_31));
					p = CTPeriod(CTRef(year - 1, OCTOBER, DAY_01), CTRef(year, SEPTEMBER, DAY_30));

				// in the original code, it was from August to September
				//CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));


				CStatistic Tair = weather.GetStat(H_TAIR, p);

				double media = Tair[MEAN];
				double ampiezza = 8;
				double shift = -30;

				size_t dd = p.Begin() - pp.Begin();
				for (size_t d = 0; d < p.size(); d++, dd++)
				{
					m_mean_T_day[dd].Tsoil = media + ampiezza * cos(PI * (d + 1 + shift) / 180);
					m_mean_T_day[dd].RC_G_Tsoil = m_P.RC_G(m_mean_T_day[dd].Tsoil);
				}
			}
		}

		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			// Setup simulation
			int year = weather[y].GetTRef().GetYear();

			CTPeriod p;
			//			p = CTPeriod(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
			if (m_version == V_ORIGINAL || m_version == V_RECALIBRATED)
				p = CTPeriod(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
			else if (m_version == V_MODIFIED)
				p = CTPeriod(CTRef(year - 1, OCTOBER, DAY_01), CTRef(year, SEPTEMBER, DAY_30));
			//p = CTPeriod(CTRef(year - 1, SEPTEMBER, DAY_01), CTRef(year, AUGUST, DAY_31));

		// in the original code, it was from August to September
		//CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));

			CDefoliation def;
			if (!m_defioliation.empty())
			{
				def.previous = m_defioliation[year - 1];
				def.current = m_defioliation[year];
			}



			// Calculate current year Bud removal percentage
			ASSERT(def.current >= 0 && def.current < 1);
			def.def = m_P.Def_min + (1 - m_P.Def_min) / (1 + pow((def.current * 100) / m_P.Def_mid, m_P.Def_slp));


			// Calculate intermediate variables
			double Mdw_0 = m_P.bud_dw * m_P.buds_num * def.def;
			double Ndw_0 = m_P.NB_r * m_P.Bdw_0 * (1 - def.previous);
			static const double I_0 = 1.0;
			double Budburst_thr = m_P.BB_thr * Mdw_0;


			// State variables initial values[s st mdw bdw C I]
			CVariables x0 = {
					m_P.S_conc_0 * (m_P.Bdw_0 + Mdw_0),
					m_P.St_conc_0 * (m_P.Bdw_0 + Mdw_0),
					Mdw_0,
					m_P.Bdw_0,
					m_P.C_0,
					I_0,
					false,
					false,
			};










			CVariables x = x0;
			size_t d = p.Begin() - pp.Begin();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++, d++)
			{

				double PS = max(0.0, min(1.0, (x.Mdw - Mdw_0) / (Budburst_thr - Mdw_0)));
				//double PS = max(0.0, min(1.0, (x.Mdw /(Budburst_thr*Mdw_0)));
				//double SDI_Dhont = cdf(SDI_dist, PS) * MAX_SDI_STAGE;//0 .. 6;
				//double SDI_Auger = max(0.0, min(5.0, exp(log(5) * (SDI_Dhont - 2.5) / (5.6 - 2.5)) - 0.33));//0 .. 5;
				//double SDI_Auger = max(0.0, min(5.0, -0.1767 + 5.5566 * (exp(-pow((6 - SDI_Dhont) / 1.9977, 1.1469)))));
				//double SDI = m_SDI_type == SDI_DHONT ? SDI_Dhont : SDI_Auger;
				double SDI = cdf(SDI_dist, PS) * 5;
				double current_branch_mass = (x.Bdw + x.Mdw) - (m_P.Bdw_0);//[g]
				//+Mdw_0

				//Bdw_0 = branch mass at the beginning of the simulation including the mass of the last years but excluding buds
				//Bdw = current year branch mass including the mass of the last years but excluding buds
				//Mdw0 = mass of 3 buds at the beginning of the simulation
				//Mdw = mass of 3 buds during the simulation


				output[TRef][O_S_CONC] = x.S / (x.Mdw + x.Bdw);//Sugars concentration [mg/g DW] 
				output[TRef][O_ST_CONC] = x.St / (x.Mdw + x.Bdw);// Starch concentration [mg/g DW]
				output[TRef][O_BRANCH_LENGTH] = max(2.3, Weight2Length(m_species, x.Bdw - m_P.Bdw_0 + x.Mdw, W2L_REMI));//[g]
				output[TRef][O_BUDS_MASS] = x.Mdw;//[g]
				output[TRef][O_BRANCH_MASS] = x.Bdw - m_P.Bdw_0;//[g]
				output[TRef][O_NEEDLE_MASS] = PS < 1 ? 0 : m_P.NB_r * current_branch_mass * (1 - def.previous);  //[g];
				//output[TRef][O_MERISTEMS] = x.Mdw;//[g]
				//output[TRef][O_BRANCH] = (x.Bdw + x.Mdw);//[g]
				//output[TRef][O_NEEDLE] = m_P.NB_r * (x.Bdw + x.Mdw - m_P.Bdw_0) * (1 - def.previous) + Ndw_0;  //[g];


				array<double, 6> Sx = SDI_2_Sx(SDI, m_bCumul);

				output[TRef][O_S5] = Round(Sx[5] * 100, 1);
				output[TRef][O_S4] = Round(Sx[4] * 100, 1);
				output[TRef][O_S3] = Round(Sx[3] * 100, 1);
				output[TRef][O_S2] = Round(Sx[2] * 100, 1);
				output[TRef][O_S1] = Round(Sx[1] * 100, 1);
				output[TRef][O_S0] = Round(Sx[0] * 100, 1);
				output[TRef][O_SDI] = SDI;


				COutputEx outputEx;


				//size_t nbSteps = m_P.m_nbSteps;
				for (size_t t = 0; t < m_nbSteps; t++)
				{
					//Phenological switches
					x.Budburst_switch |= (x.Mdw >= Budburst_thr);
					x.Swell_switch |= (x.S / (x.Mdw + x.Bdw) >= 80);
					//x.Swell_switch = TRef.GetYear() == p.End().GetYear();//swelling only at the second year


					//double f = 1.0 - (t - 1.0) / (nbSteps - 1.0);
					//CInput I = input[d] * f + input[d + 1] * (1 - f);
					CVariables dx = PhenologyConiferEquations(m_mean_T_day[d], x, m_P, def, outputEx, bModelEx);
					x = x + dx / m_nbSteps;

					//limit concentration to valid values
					//double S_conc = max(P.S_min, min(P.S_max, x.S / (x.Mdw + x.Bdw)));       // Sugars concentration [mg/g DW]
					//double St_conc = P.St_min + x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]
					//update sugar and starch
					//x.S = S_conc * (x.Mdw + x.Bdw);
					//x.St = St_conc * (x.Mdw + x.Bdw);
					//x.S = max(1.0, x.S);
					//x.St = max(1.0, x.St);

					x.limitToZero();
				}




				if (bModelEx)
				{

					output[TRef][O_C] = x.C;
					output[TRef][O_INHIBITOR] = x.I;
					output[TRef][O_SUGAR] = x.S;//[mg]
					output[TRef][O_STARCH] = x.St;//[mg] 
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
					output[TRef][O_SWELL_SWITCH] = x.Swell_switch;
					output[TRef][O_TAIR] = m_mean_T_day[d].Tair;
					output[TRef][O_TSOIL] = m_mean_T_day[d].Tsoil;
					output[TRef][O_PN] = m_mean_T_day[d].PN;
					output[TRef][O_RC_G_TAIR] = m_mean_T_day[d].RC_G_Tair;
					output[TRef][O_RC_F_TAIR] = m_mean_T_day[d].RC_F_Tair;
					output[TRef][O_RC_M_TAIR] = m_mean_T_day[d].RC_M_Tair;
					output[TRef][O_RC_G_TSOIL] = m_mean_T_day[d].RC_G_Tsoil;
					output[TRef][O_BUDBURST] = x.Budburst_switch;
				}
				
			}


			//update S_conc_0 and St_conc_0
			//double S_conc = max(P.S_min, min(P.S_max, x.S / (x.Mdw + x.Bdw)));       // Sugars concentration [mg/g DW]
			//double St_conc = P.St_min + x.St / (x.Mdw + x.Bdw);     // Starch concentration [mg/g DW]

			//P.S_conc_0 = x.S / (x.Mdw + x.Bdw);
			//P.St_conc_0 = x.St / (x.Mdw + x.Bdw);

			//P.S_conc_0 = x0.S;
			//P.St_conc_0 = x0.St;

		}


		return msg;
	}


}