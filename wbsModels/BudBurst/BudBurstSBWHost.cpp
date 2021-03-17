//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstSBWHost.h"
#include "PHENO_eqs.h"
#include <deque>


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::HBB;

namespace WBSF
{
	CSBWHostBudBurst::CSBWHostBudBurst()
	{
	}

	CSBWHostBudBurst::~CSBWHostBudBurst()
	{
	}

	//This method is call to compute solution

	ERMsg CSBWHostBudBurst::Execute(CWeatherStation& weather, CModelStatVector& output)
	{
		ASSERT(weather.IsHourly());
		ASSERT(weather.GetNbYears() >= 2);
		ASSERT(m_species < NB_SBW_SPECIES);

		ERMsg msg;

		CParameters P = PARAMETERS[m_species];

		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		output.Init(pp, NB_HBB_OUTPUTS, -999);



		//compute input
		// Resample Daily Climate
		deque<CInput> input(pp.size());
		for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
				{
					CStatistic PN_stat;
					CStatistic Tair_stat;
					for (size_t h = 0; h < 24; h++)
					{
						//array <double, 24> PN;
						const CHourlyData& hData = weather[y][m][d][h];

						//Calculate unitary ps
						//convert total solar radiation (SRAD) [Watt/m²] into photo active radiation (PAR) μmol*m²/s
						double PAR = hData[H_SRAD] * 2.1;
						//The approximation 1 Watt/m² ≈ 4.57 μmol.m2/s comes from the Plant Growth Chamber Handbook(chapter 1, radiation; https://www.controlledenvironments.org/wp-content/uploads/sites/6/2017/06/Ch01.pdf). 
						//Note that the value of 4.57 converts Watts/m2 to μmol.m2/s, assuming that the Watt/m² is for radiation from 400 to 700 nm. 
						//However, I don't know that anyone ever measures solar radiation in Watt/m² in the 400-700 nm range. 
						//Pyranometers measure total solar radiation. 
						//Since only about 45% of the energy of solar radiation is actually in the 400-700 nm range. 
						//the conversion of total solar radiation to PAR is ~2.1, rather than 4.57. 
						//As mentioned by others, that is an approximation, but a pretty good one.

						double PN_PAR = P.PS_PAR_1 * (1 - pow(1 - P.PS_PAR_3 / P.PS_PAR_2, 1 - PAR / P.PS_PAR_2)) + abs(P.PS_PAR_3);
						double RC_PS_T = P.RC_PS(hData[H_TAIR]);

						static const double umol2mgC = 0.029526111;
						double PN = PN_PAR * RC_PS_T * 3600 * P.SLA;    // umolCO2 gDW - 1 h - 1
						PN *= umol2mgC; // mgSugar gDW - 1 d - 1
						PN_stat += PN;

						Tair_stat += hData[H_TAIR];
					}

					input[dd].Tair = Tair_stat[MEAN];
					input[dd].PN = PN_stat[SUM];

				}
			}
		}


		// Estimate soil temperature from air temperature
		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			int year = weather[y].GetTRef().GetYear();
			//CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, JULY, DAY_31));
			CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));
			CStatistic Tair = weather.GetStat(H_TAIR, p);

			double media = Tair[MEAN];
			double ampiezza = 8;
			double shift = -30;

			size_t dd = p.Begin() - pp.Begin();
			for (size_t d = 0; d < p.size(); d++, dd++)
				input[dd].Tsoil = media + ampiezza * cos(PI*(d +1 + shift) / 180);
		}


		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			// Setup simulation
			int year = weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year - 1, AUGUST, DAY_01), CTRef(year, AUGUST, DAY_31));

			CDefoliation def;
			def.previous = m_defioliation[year - 1];
			def.current = m_defioliation[year];


			// Calculate current year Bud removal percentage
			//double defol = P.Def(def.current);
			def.def = P.dmin + (P.dmax - P.dmin) / (1 + pow((def.current * 100) / P.mid, P.slope));


			// Calculate intermediate variables
			double Mdw_0 = P.bud_dw * P.buds_num*def.def;
			//double Mdw_0 = P.Mdw_0(def.current);
			double Ndw_0 = P.r_nb * P.Bdw_0*(1 - def.previous);
			//double Budburst_thr = P.Budburst_thr(def.current);
			//double Budburst_thr = P.Budburst_thr(def.current);
			double Budburst_thr = P.BB_thr*Mdw_0;
			P.Veg_switch = false;
			P.Swell_switch = false;


			// State variables initial values[s st mdw bdw C I]
			CVariables x0 = {
					P.S_0 * (P.Bdw_0 + Mdw_0),
					P.St_0 * (P.Bdw_0 + Mdw_0),
					Mdw_0,
					P.Bdw_0,
					P.C_0,
					1
			};


			CVariables x = x0;
			size_t d = p.Begin() - pp.Begin();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++, d++)
			{
				output[TRef][O_SUGAR] = x.S;//[mg/g]
				output[TRef][O_STARCH] = x.St;//[mg/g]
				output[TRef][O_MERISTEMS] = x.Mdw;//[g]
				output[TRef][O_BRANCH] = x.Bdw + x.Mdw;//[g]
				output[TRef][O_NEEDLE] = P.r_nb * (x.Bdw + x.Mdw - P.Bdw_0)*(1 - def.previous) + Ndw_0;  //[g];
				output[TRef][O_C] = x.C;
				output[TRef][O_INHIBITOR] = x.I;
				output[TRef][O_BUDBURST] = min(100.0, (x.Mdw - Mdw_0) / (Budburst_thr - Mdw_0) * 100);//[%]
				//output[TRef][O_BUDBURST] = P.Veg_switch ?100:0;//[%]
				


				size_t nbSteps = 10;
				for (size_t t = 0; t < nbSteps; t++)
				{
					//Phenological switches
					P.Veg_switch |= (x.Mdw >= Budburst_thr);
					P.Swell_switch |= x.S / (x.Mdw + x.Bdw) >= 200;

					double f = 1.0 - (t - 1.0) / (nbSteps - 1.0);
					//CInput I = input[d] * f + input[d + 1] * (1 - f);
					CVariables dx = PhenologyConiferEquations(input[d], x, P, def);
					x = x + dx / nbSteps;
					x.limitToZero();
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