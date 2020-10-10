//  F32.c  written by BMW @ PNFI
//Modifications
// 01/01/1994  precision fix etc........bmw
// 01/10/2002
//     to eliminate negative evapouration from DC skit in November
//     thru March.  Change of code to match equations in FTR-33 (1985) 
//     report (change from F32.FOR)
// 01/06/2005  to add STDLIB.h to solve recent problems in conversion of startups
//    from char to float
// 10/12/2007 	Rémi Saint-Amant	Incorporate in BioSIMModelBase
// 01/02/2009	Rémi Saint-Amant	Use new snow model to predic last snow day
// 27/02/2009	Rémi Saint-Amant	Include new variable for montly model
// 05/03/2010	Rémi Saint-Amant	Include overwintering DC and continious mode when no freezing
// 11/03/2010	Rémi Saint-Amant	Include start threshold
// 18/05/2011   Rémi Saint-Amant	Change result format( January 1 to December 31), correction of a bug in computing new DC
//									Add of m_carryOverFraction and EFFECTIVENESS_OF_WINTER as static parameters
// 06/02/2012	Rémi Saint-Amant	Correction of a bug in the first date. The first date must start 3 days after the snow melt 
// 05/09/2016	Rémi Saint-Amant	Many modifications to support hourly inputs
// 16/03/2020   Rémi Saint-Amant	Add initial values from file
//**********************************************************************
#include <math.h>
#include "Basic/CSV.h"
#include "Basic/GrowingSeason.h"
#include "Basic/SnowAnalysis.h"
#include "Basic/WeatherDefine.h"
#include "Basic/UtilMath.h"
#include "FWI.h"
//******************************************


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//**************************************************************************************************
	//CInitialValues

	ERMsg CInitialValues::Load(const std::string& data)
	{
		ASSERT(!data.empty());

		ERMsg msg;

		std::stringstream stream(data);


		for (CSVIterator loop(stream); loop != CSVIterator() && msg; ++loop)
		{
			enum TInput { I_KEYID, I_START_DATE, I_FFMC, I_DMC, I_DC, I_END_DATE/*Optionnal*/, NB_INPUT_COLUMNS };

			if (loop->size() == NB_INPUT_COLUMNS - 1 ||
				loop->size() == NB_INPUT_COLUMNS)
			{
				CTRef TRef;
				TRef.FromFormatedString((*loop)[I_START_DATE]);

				if (TRef.IsValid())
				{
					double begin_DOY = TRef.GetJDay();
					double end_DOY = -1;
					if (loop->size() == NB_INPUT_COLUMNS)
					{
						CTRef TRef;
						TRef.FromFormatedString((*loop)[I_END_DATE]);
						if (TRef.IsValid())
							end_DOY = TRef.GetJDay();
						else
							msg.ajoute("Invalid end date " + (*loop)[I_END_DATE]);
					}


					string name = (*loop)[I_KEYID] + "_" + to_string(TRef.GetYear());
					(*this)[name] = { {begin_DOY, stod((*loop)[I_FFMC]), stod((*loop)[I_DMC]), stod((*loop)[I_DC]), end_DOY } };
				}
				else
				{
					msg.ajoute("Invalid start date " + (*loop)[I_START_DATE]);
				}
			}
		}

		return msg;

	}

	//**************************************************************************************************
	//CFWI

	CFWI::CFWI()
	{
		Reset();
	}

	CFWI::~CFWI()
	{}

	void CFWI::Reset()
	{
		m_method = NOON_CALCULATION;

		m_bAutoSelect = true;
		m_nbDaysStart = 3;
		m_TtypeStart = CGSInfo::TT_TNOON;
		m_thresholdStart = 12;

		m_nbDaysEnd = 3;
		m_TtypeEnd = CGSInfo::TT_TMAX;
		m_thresholdEnd = 5;

		m_firstDay = 0;
		m_lastDay = 365;
		m_FFMC = 85.0;
		m_DMC = 6.0;
		m_DC = 15.0;

		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;
	}

	//double CFWI::GetHFFMC(double oldFFMC, const CHourlyData& data)
	//{
	//	return GetHFFMC(oldFFMC, data[H_TAIR], data[H_RELH], data[H_WNDS], data[H_PRCP]);
	//}

	//double CFWI::GetFFMC(double oldFFMC, const CWeatherDay& data, double prcp)
	//{
	//	ASSERT(data.IsHourly());
	//	return GetFFMC(oldFFMC, data[12][H_TNTX], data[12][H_RELH], data[12][H_WNDS], prcp/*data[H_PRCP][SUM]*/);
	//}




	//T: Temperature (°C)
	//Hr: relative humidity (%)
	//Ws: wind speed (km/h)
	//prcp: precipitation in mm
	double CFWI::GetFFMC(double oldFFMC, double T, double Hr, double Ws, double prcp)
	{
		double wmo = 147.2*(101 - oldFFMC) / (59.5 + oldFFMC);
		if (prcp > 0.5)
		{
			double ra = prcp - 0.5;
			if (wmo > 150)
				wmo += 42.5*ra*exp(-100.0 / (251 - wmo))*(1.0 - exp(-6.93 / ra)) + 0.0015*(wmo - 150)*(wmo - 150)*sqrt(ra);
			else wmo += 42.5*ra*exp(-100.0 / (251 - wmo))*(1.0 - exp(-6.93 / ra));
		}

		if (wmo > 250)
			wmo = 250;

		double temp = T;
		double ed = 0.942*pow(Hr, 0.679) + (11.0*exp((Hr - 100.0) / 10.0)) + 0.18*(21.1 - temp)*(1.0 - 1.0 / exp(Hr*0.115));
		double ew = 0.618*pow(Hr, 0.753) + (10.0*exp((Hr - 100.0) / 10.0)) + 0.18*(21.1 - temp)*(1.0 - 1.0 / exp(Hr*0.115));

		double wm = wmo;
		if (wm < ed && wm < ew)
		{
			double z = 0.424*(1.0 - pow(((100.0 - Hr) / 100.0), 1.7)) + 0.0694*sqrt(Ws)*(1.0 - pow((100.0 - Hr) / 100.0, 8.0));
			double x = z * 0.581*exp(0.0365*temp);
			wm = ew - (ew - wmo) / pow(10.0, x);
		}
		else if (wm > ed)
		{
			double z = 0.424*(1.0 - pow((Hr / 100.), 1.7)) + 0.0694*sqrt(Ws)*(1 - pow(Hr / 100, 8.0));
			double x = z * 0.581*exp(0.0365*temp);
			wm = ed + (wmo - ed) / pow(10.0, x);
		}


		double ffmc = max(0.0, min(101.0, 59.5*(250.0 - wm) / (147.2 + wm)));

		return ffmc;
	}


	//compute from: A comparison of hourly fine fuel moisture code calculations within canada
	//Kerry Anderson, Canadian Forest Service, 2009
	double CFWI::GetHFFMC(double Fo, double T, double Hr, double Ws, double ro)
	{
		double mo = 205.2*(101.0 - Fo) / (82.9 + Fo);//after Anderson 2009
		//double mo = 147.27723*(101 - Fo) / (59.5 + Fo);

		if (ro > 0)
		{
			double mr = 0;
			if (mo <= 150.0)
			{
				//[7a] m = mn + 42.5 r(e " 1 0 ° / ( 2 5 1 ~ m0) ) (1 - « r 6 ' 9 3 / r o ) .r o o : \, mQ £ 150
				mr = mo + 42.5*ro*exp(-100.0 / (251.0 - mo))* (1 - exp(-6.93 / ro));
			}
			else
			{
				//[7b] mr = mQ + 42.5 r o(e ~1 0 0 / *2 5 1 ~' % h (1 - e " 6*9 3 / ro)+ 0.0015(mQ - 150) r° * 5 , mQ > 150
				mr = mo + 42.5*ro*exp(-100.0 / (251.0 - mo))* (1 - exp(-6.93 / ro)) + 0.0015*(mo - 150.0)* sqrt(ro);
			}

			mo = mr;
		}

		
		if (mo > 250)
			mo = 250;


		double m = 0;
		//[2a] Ed = 0.942 H 0 * 6 7 9 + 11 - 1 0 0 ) / 1 0 + 0.18(21.1 - T)(1 - e(H - 100) / 10- 0.115 H
		double Ed = 0.942*pow(Hr, 0.679) + 11.0*exp((Hr - 100.0) / 10.0) + 0.18*(21.1 - T)*(1 - exp(-0.115*Hr));
		if (mo > Ed)
		{
			//[3a] ka = 0.424[1 - (H / 100)1 * 7] + 0.0694 W0'5 [1 - (H/100)8]a+ 10 e + 0.18(21.1 - T)(1 - e
			double Ka = 0.424*(1.0 - pow(Hr / 100.0, 1.7)) + 0.0694*pow(Ws, 0.5)*(1.0 - pow(Hr / 100.0, 8.0));
			//[3b] k 0.0365 T d = 0.0579 kfle
			double Kd = 0.0579*Ka*exp(0.0365*T);

			//[5a] m = Ed + (mo - Ed) e ' 2 - 3 0 3 kd
			m = Ed + (mo - Ed)*exp(-2.303*Kd);
		}
		else
		{
			//[2b] E, = 0.618 H0 .753 - 0.115 Hw a+ 10 e + 0.18(21.1 - T)(1 - e;
			double Ew = 0.618*pow(Hr, 0.753) + 10.0*exp((Hr - 100.0) / 10.0) + 0.18*(21.1 - T)*(1 - exp(-0.115*Hr));
			if (mo < Ew)
			{
				//[4a] k 1.7 0.5 8 b = 0.424[1 - ({ TOO. - H) / 100)] + 0.0694 W[1 - ((100 - H) / l00)]
				double Kb = 0.424*(1 - pow((100.0 - Hr) / 100.0, 1.7)) + 0.0694 *sqrt(Ws)*(1 - pow((100 - Hr) / 100.0, 8.0));
				//[4b] k = 0.0579 k.e 0.0365 T w b
				double Kw = 0.0579*Kb*exp(0.0365*T);

				//[5b] m = Ew - (Ew - mo) e " 2 ' 3 0 3 "S /
				m = Ew + (Ew - mo)*exp(-2.303*Kw);
			}
			else
			{
				m = mo;
			}
		}


		//[6] F = 59.5(250 - m) / (147.27723 + m)
		//double F = 59.5*(250 - m) / (147.27723 + m);//after alexander 1984
		double F = 82.9*(250 - m) / (205.2 + m);//after  anderson 2009

		return max(0.0, min(101.0, F));
	}

	//double CFWI::GetDMC(double oldDMC, const CHourlyData& data)
	//{
	//	return GetDMC(oldDMC, data.GetTRef().GetMonth(), data[H_TAIR], data[H_RELH], data[H_PRCP]);
	//}

	//double CFWI::GetDMC(double oldDMC, const CWeatherDay& data, double prcp)
	//{
	//	ASSERT(data.IsHourly());
	//	return GetDMC(oldDMC, data.GetTRef().GetMonth(), data[12][H_TNTX], data[12][H_RELH], prcp/*data[H_PRCP][SUM]*/);
	//}


	double CFWI::GetDMC(double oldDMC, size_t m, double T, double Hr, double prcp)
	{
		//short m = day.Month();
		static const double el[12] = { 6.5, 7.5, 9.0, 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8.0, 7.0, 6.0 };

		double temp = T;
		double t = max(-1.1, temp);

		double rk = 1.894*(t + 1.1)*(100.0 - Hr)*el[m] * 0.0001;

		_ASSERTE(oldDMC >= 0);
		double pr = oldDMC;
		if (prcp > 1.5)
		{
			double ra = prcp;
			double rw = 0.92*ra - 1.27;
			double wmi = 20.0 + 280.0 / exp(0.023*oldDMC);

			double b = 6.2*log(oldDMC) - 17.2;
			if (oldDMC <= 33)
				b = 100.0 / (0.5 + 0.3*oldDMC);
			else if (oldDMC <= 65)
				b = 14.0 - 1.3*log(oldDMC);

			double wmr = wmi + 1000.0*rw / (48.77 + b * rw);
			pr = max(0.0, 43.43*(5.6348 - log(wmr - 20.0)));
		}

		double dmc = max(0.0, pr + rk);

		return dmc;
	}

	//double CFWI::GetDC(double oldDC, const CHourlyData& data)
	//{
	//	return GetDC(oldDC, data.GetLocation().m_lat, data.GetTRef().GetMonth(), data[H_TAIR], data[H_PRCP]);
	//}

	//double CFWI::GetDC(double oldDC, const CWeatherDay& data, double prcp)
	//{
	//	ASSERT(data.IsHourly());
	//	return GetDC(oldDC, data.GetLocation().m_lat, data.GetTRef().GetMonth(), data[12][H_TNTX], prcp/*data[H_PRCP][SUM]*/);
	//}

	double CFWI::GetDC(double oldDC, double lat, size_t m, double T, double prcp)
	{
		ASSERT(oldDC >= 0);

		//Day length factor for DC Calculations
		//20N: North of 20 degrees N
		static const double flN[12] = { -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6 };
		//20S: South of 20 degrees S
		static const double flS[12] = { 6.4, 5, 2.4, 0.4, -1.6, -1.6, -1.6, -1.6, -1.6, 0.9, 3.8, 5.8 };

		double temp = T;
		double t = max(-2.8, temp);
		//Daylength factor adjustment by latitude for Potential Evapotranspiration
		double fl = lat <= -20 ? flS[m] : lat > 20 ? flN[m] : 1.4;
		double pe = max(0.0, (0.36*(t + 2.8) + fl) / 2.0);
		//double pe = max(0.0, (0.36*(t + 2.8) + fl[m]) / 2.0);



		double dr = oldDC;
		if (prcp > 2.8)
		{
			double ra = prcp;
			double rw = 0.83*ra - 1.27;
			double smi = 800 * exp(-oldDC / 400);
			dr = max(0.0, oldDC - 400.0*log(1.0 + 3.937*rw / smi));
		}

		double dc = max(0.0, dr + pe);

		return dc;
	}
/*
	double CFWI::GetHISI(double FFMC, const CHourlyData& data)
	{
		return GetHISI(FFMC, data[H_WNDS]);
	}

	double CFWI::GetISI(double FFMC, const CWeatherDay& data)
	{
		return GetISI(FFMC, data[12][H_WNDS]);
	}
*/
	//FFMC:   Fine Fuel Moisture Code
	//Ws : Wind Speed(km/h)
	//fbpMod : TRUE / FALSE if using the fbp modification at the extreme end
	//Returns ISI:    Initial Spread Index
	double CFWI::GetISI(double FFMC, double Ws, bool fbpMod)
	{
		/*double fm = 147.2*(101.0 - FFMC) / (59.5 + FFMC);
		double sf = 91.9*exp(-0.1386*fm)*(1.0 + pow(fm, 5.31) / 4.93e07);
		double isi = 0.208*sf * exp(0.05039*Ws);

		return isi;*/

		//Eq. 10 - Moisture content
		double fm = 147.2 * (101 - FFMC) / (59.5 + FFMC);
		//Eq. 24 - Wind Effect
		//the ifelse, also takes care of the ISI modification for the fbp functions
		// This modification is Equation 53a in FCFDG(1992)
		double fW = (Ws >= 40 && fbpMod == TRUE) ? 12 * (1 - exp(-0.0818 * (Ws - 28))) : exp(0.05039 * Ws);
		//Eq. 25 - Fine Fuel Moisture
		double fF = 91.9 * exp(-0.1386 * fm) * (1.0 + pow(fm, 5.31) / 4.93e07);
		//Eq. 26 - Spread Index Equation
		double isi = 0.208 * fW * fF;

		return isi;
	}

	double CFWI::GetHISI(double Fo, double Ws, bool fbpMod)
	{

		//double m = 205.2*(101.0 - Fo) / (82.9 + Fo);//after anderson 2009
		//double sf = 91.9*exp(-0.1386*m)*(1.0 + pow(m, 5.31) / 4.93e07);
		//double isi = 0.208*sf * exp(0.05039*Ws);

		//return isi;

		//Eq. 10 - Moisture content
		double fm = 205.2*(101.0 - Fo) / (82.9 + Fo);//after anderson 2009
		//double fm = 147.27723 * (101.0 - Fo) / (59.5 + Fo);
		//Eq. 24 - Wind Effect
		//the ifelse, also takes care of the ISI modification for the fbp functions
		// This modification is Equation 53a in FCFDG(1992)
		double fW = (Ws >= 40 && fbpMod == TRUE) ? 12 * (1 - exp(-0.0818 * (Ws - 28))) : exp(0.05039 * Ws);
		//Eq. 25 - Fine Fuel Moisture
		double fF = 91.9 * exp(-0.1386 * fm) * (1.0 + pow(fm, 5.31) / 4.93e07);
		//Eq. 26 - Spread Index Equation
		double isi = 0.208 * fW * fF;

		return isi;
	}

	double CFWI::GetBUI(double dmc, double dc)
	{
		double bui = 0;

		if (dmc == 0 && dc == 0) bui = 0;
		else bui = 0.8*dc*dmc / (dmc + 0.4*dc);

		if (bui < dmc)
		{
			double p = (dmc - bui) / dmc;
			double cc = 0.92 + pow((0.0114*dmc), 1.7);
			bui = max(0.0, dmc - cc * p);
		}

		return bui;
	}

	double CFWI::GetFWI(double bui, double isi)
	{
		double bb = 0;
		if (bui > 80)
			bb = 0.1*isi*(1000.0 / (25.0 + 108.64 / exp(0.023*bui)));
		else
			bb = 0.1*isi*(0.626*pow(bui, 0.809) + 2.0);

		double fwi = bb;
		if (fwi > 1)
			fwi = exp(2.72*pow(0.434*log(fwi), 0.647));

		return fwi;
	}

	double CFWI::GetDSR(double fwi)
	{
		double dsr = 0.0272*pow(fwi, 1.77);
		return dsr;
	}

	//convert from site: https://rdrr.io/cran/cffdrs/src/R/FMCcalc.r
	// if D0, date of min FMC, is not known then D0 = 0.
	//############################################################################
	// Description:
	//   Calculate Foliar Moisture Content on a specified day.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   lat : Latitude(decimal degrees)
	//   lon : Longitude(decimal degrees)
	//   alt : Elevation(metres)
	//   DJ : Day of year(offeren referred to as julian date)
	//   D0 : Date of minimum foliar moisture content
	//   
	// Returns :
	//   FMC : Foliar Moisture Content
	//
	//############################################################################
	double FMC(double lat, double lon, double elev, size_t DJ, double D0 = 0)
	{
		//Calculate Normalized Latitude
		//Eqs. 1 & 3 (FCFDG 1992)
		if (D0 <= 0)
		{
			if (elev == -999)
			{
				double LATN = 46 + 23.4 * exp(-0.0360 * (150 - lon));
				//Calculate Date of minimum foliar moisture content
				//Eqs. 2 & 4 (FCFDG 1992): Round D0 to the nearest integer because it is a date
				D0 = Round(151 * (lat / LATN), 0);
			}
			else
			{
				double LATN = 43 + 33.7 * exp(-0.0351 * (150 - lon));
				//Calculate Date of minimum foliar moisture content
				//Eqs. 2 & 4 (FCFDG 1992): Round D0 to the nearest integer because it is a date
				D0 = Round(142.1 * (lat / LATN) + 0.0172 * elev);
			}

			//double LATN = (elev == -999) ? 46 + 23.4 * exp(-0.0360 * (150 - lon)) : 43 + 33.7 * exp(-0.0351 * (150 - lon));
			////Calculate Date of minimum foliar moisture content
			////Eqs. 2 & 4 (FCFDG 1992)
			//D0 = (elev <= 0) ? 151 * (lat / LATN) : 142.1 * (lat / LATN) + 0.0172 * elev;
			////Round D0 to the nearest integer because it is a date
			//D0 = Round(D0, 0);
		}

		//Number of days between day of year and date of min FMC
		//Eq. 5 (FCFDG 1992)
		int ND = abs(int(DJ) - int(D0));
		//Calculate final FMC
		//Eqs. 6, 7, &8 (FCFDG 1992)
		double FMC = (ND < 30) ? 85.0 + 0.0189 * ND *ND :
			(ND >= 30 && ND < 50) ? 32.9 + 3.17 * ND - 0.0288 * ND*ND : 120;

		return FMC;
	}


	// Description: Calculation of the Grass Fuel Moisture Code.This calculates
	//              the moisture content of both the surface of a fully cured
	//              matted grass layer and also an equivalent Grass Fuel Moisture
	//              Code.All equations come from Wotton(2009) as cited below
	//              unless otherwise specified.
	// 
	//              Wotton, B.M. 2009. A grass moisture model for the Canadian
	//              Forest Fire Danger Rating System.In: Proceedings 8th Fire and
	//              Forest Meteorology Symposium, Kalispell, MT Oct 13 - 15, 2009.
	//              Paper 3 - 2. https://ams.confex.com/ams/pdfpapers/155930.pdf
	//           
	// Args : input(data.frame) :
	//         temp(required)	    Temperature(centigrade)
	//         rh(required)	    Relative humidity(%)
	//         ws(required)	    10 - m height wind speed(km / h)
	//         prec(required)	    1 - hour rainfall(mm)
	//         isol(required)	    Solar radiation(kW / m ^ 2)
	//         mon(recommended)	Month of the year(integer 1 - 12)
	//         day(optional)	    Day of the month(integer)
	//       GFMCold:    GFMC from yesterday(double, default = 85)
	//       batch : Compute multiple locations(TRUE / FALSE, default = TRUE)
	//       time.step:  The hourly time steps(integer hour, default = 1)
	//       roFL : Nominal fuel load of the fine fuel layer
	//                   (kg/m^2 double, default=0.3)
	//       out : Output format(GFMCandMC / MC / GFMC / ALL, default = GFMCandMC)
	//       
	// Returns : Returns a data.frame of either MC, GMFC, All, or GFMCandMC
	//


	double gfmc(double input, double GFMCold = 85, bool batch = TRUE, double time_step = 1, double roFL = 0.3, string out = "GFMCandMC")
	{
		/*
		t0 =time.step
			names(input) =tolower(names(input))
			//Quite often users will have a data frame called "input" already attached
			//  to the workspace.To mitigate this, we remove that if it exists, and warn
			//  the user of this case.
			if (!is.na(charmatch("input", search()))) {
				warning("Attached dataset 'input' is being detached to use fbp() function.")
					detach(input)
			}
		//set local scope variables
		temp =input$temp
			prec =input$prec
			ws =input$ws
			rh =input$rh
			isol =input$isol

			//show warnings when inputs are missing
			if (!exists("temp") | is.null(temp))
				warning("temperature (temp) is missing!")
				if (!exists("prec") | is.null(prec))
					warning("precipitation (prec) is missing!")
					if (!exists("ws") | is.null(ws))
						warning("wind speed (ws) is missing!")
						if (!exists("rh") | is.null(rh))
							warning("relative humidity (rh) is missing!")
							if (!exists("isol") | is.null(isol))
								warning("ISOL is missing!")

								//check for issues with batching the function
								if (batch) {
									if ("id" %in% names(input)) {
										n =length(unique(input$id))
											if (length(unique(input[1:n, "id"])) != n) {
												stop("Multiple stations have to start and end at the same dates/time,
													and input data must be sorted by date / time and id")
											}
									}
									else {
										n =1
									}
								}
								else { n =nrow(input) }

		if (length(temp) % %n != 0)
			warning("Input data do not match with number of weather stations")

			if (length(GFMCold) != n & length(GFMCold) == 1) {
				warning("One GFMCold value for multiple weather stations")
					GFMCold =rep(GFMCold, n)
			}

		if (length(GFMCold) != n & length(GFMCold) > 1)
			stop("Number of GFMCold doesn't match number of wx stations")
			validOutTypes = c("GFMCandMC", "MC", "GFMC", "ALL")
			if (!(out %in% validOutTypes)) {
				stop(paste("'", out, "' is an invalid 'out' type.", sep = ""))
			}

		//get the length of the data stream
		n0 =length(temp) % / %n
			GFMC =NULL
			MC =NULL
			//iterate through timesteps
			for (i in 1 : n0) {
				//k is the data for all stations by time step
				k =(n * (i - 1) + 1) : (n * i)
					//Eq. 13 - Calculate previous moisture code
					MCold =147.2772 * ((101 - GFMCold) / (59.5 + GFMCold))
					//Eq. 11 - Calculate the moisture content of the layer in % after rainfall
					MCr =ifelse(prec[k] > 0, MCold + 100 * (prec[k] / roFL), MCold)
					//Constrain to 250
					MCr =ifelse(MCr > 250, 250, MCr)
					MCold =MCr
					//Eq. 2 - Calculate Fuel temperature
					Tf =temp[k] + 35.07 * isol[k] * exp(-0.06215 * ws[k])
					//Eq. 3 - Calculate Saturation Vapour Pressure(Baumgartner et a. 1982)
					eS.T =6.107 * 10 ^ (7.5 * temp[k] / (237 + temp[k]))
					//Eq. 3 for Fuel temperature
					eS.Tf =6.107 * 10 ^ (7.5 * Tf / (237 + Tf))
					//Eq. 4 - Calculate Fuel Level Relative Humidity
					RH.f =rh[k] * (eS.T / eS.Tf)
					//Eq. 7 - Calculate Equilibrium Moisture Content for Drying phase
					EMC.D =(1.62 * RH.f^0.532 + 13.7 * exp((RH.f - 100) / 13.0)) +
					0.27 * (26.7 - Tf) * (1 - exp(-0.115 * RH.f))
					//Eq. 7 - Calculate Equilibrium Moisture Content for Wetting phase
					EMC.W =(1.42 * RH.f^0.512 + 12.0 * exp((RH.f - 100) / 18.0)) +
					0.27 * (26.7 - Tf) * (1 - exp(-0.115 * RH.f))
					//RH in terms of RH / 100 for desorption
					Rf =ifelse(MCold > EMC.D, RH.f / 100, rh)
					//RH in terms of 1 - RH / 100 for absorption
					Rf =ifelse(MCold < EMC.W, (100 - RH.f) / 100, Rf)
					//Eq. 10 - Calculate Inverse Response time of grass(hours)
					K.GRASS =0.389633 * exp(0.0365 * Tf) * (0.424 * (1 - Rf ^ 1.7) + 0.0694 *
						sqrt(ws[k]) * (1 - Rf ^ 8))
					//Fuel is drying, calculate Moisture Content
					MC0 =ifelse(MCold > EMC.D, EMC.D + (MCold - EMC.D) *
						exp(-1.0 * log(10.0) * K.GRASS * t0), MCold)
					//Fuel is wetting, calculate moisture content
					MC0 =ifelse(MCold < EMC.W, EMC.W + (MCold - EMC.W) *
						exp(-1.0 * log(10.0) * K.GRASS * t0), MC0)

					//Eq. 12 - Calculate GFMC
					GFMC0 =59.5 * ((250 - MC0) / (147.2772 + MC0))
					//Keep current and old GFMC
					GFMC =c(GFMC, GFMC0)
					//Same for moisture content
					MC =c(MC, MC0)
					//Reset vars
					GFMCold =GFMC0
					MCold =MC0
			}
		//Return requested 'out' type
		if (out == "ALL") {
			return(as.data.frame(cbind(input, GFMC, MC)))
		}
		else if (out == "GFMC") {
			return(GFMC)
		}
		else if (out == "MC") {
			return(MC)
		}
		else {
			//GFMCandMC
			return(data.frame(GFMC = GFMC, MC = MC))
		}
		*/
		return 0;
	}

	//If there is at lean 75% of the day of January an February that have at least 1cm
	//and there is at least 10 cm then hte first day is 3 days after the snowmelt date
	CTRef CFWI::GetFirstDay(const CWeatherYear& weather)
	{
		CTRef firstDate;


		int nbDay = 0;
		double maxSnow = -1;

		CTPeriod period = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CTRef end(period.Begin().GetYear(), JULY, 15);
		for (CTRef TRef = period.Begin(); TRef < end; TRef++)
		{
			if (weather[TRef][H_SNDH][MEAN] > maxSnow)
				maxSnow = weather[TRef][H_SNDH][MEAN];

			if (TRef.GetJDay() < 61 && weather[TRef][H_SNDH][MEAN] > 1)
				nbDay++;
		}


		double snowPropJF = nbDay / 61.0;
		double MINIMUM_SNOW = 10;//in cm
		if (maxSnow >= MINIMUM_SNOW && snowPropJF > 0.75)
		{
			CSnowAnalysis snow;
			firstDate = snow.GetLastSnowTRef(weather) + 3;//3 day after snow melt
		}

		return firstDate;
	}

	CTRef CFWI::GetLastDay(const CWeatherYear& weather)
	{
		//find begin of snow
		CSnowAnalysis snow;
		CTRef TRefSnow = snow.GetFirstSnowTRef(weather);

		CGrowingSeason GS(0, 0, 0, m_TtypeEnd, m_nbDaysEnd, m_thresholdEnd);
		CTPeriod p = GS.GetGrowingSeason(weather);

		//find freeze-up
		CTRef TRefFreezeUp = p.End();

		CTPeriod period = weather.GetEntireTPeriod();
		CTRef TRef = period.End();

		if (TRefSnow.IsInit())
			TRef = min(TRefSnow, TRef);

		if (TRefFreezeUp.IsInit())
			TRef = min(TRefFreezeUp, TRef);


		return TRef;

	}


	size_t CFWI::GetNbDayLastRain(const CWeatherYear& weather, size_t firstDay)
	{
		size_t nbDay = 0;
		for (size_t jd = firstDay; jd >= 0; jd--, nbDay++)
		{
			//Get the last day with at least 2mm of water
			if (weather.GetDay(jd)[H_PRCP][SUM] > 2)
			{
				break;
			}
		}

		return nbDay;
	}

	size_t CFWI::GetInitialValue(const CWeatherStation& weather, size_t y, size_t lastDay, double& FFMC, double& DMC, double& DC)
	{
		double lastDC = DC;

		size_t firstDay = GetFirstDay(weather[y]).GetJDay();

		if (firstDay >= 0)
		{
			FFMC = 85;
			DMC = 6;
			DC = 15;
		}
		else
		{
			//no snow
			//Find 3 consecutives days where noon temperature is above m_thresholdStart (12°C)
			//firstDay = GetFirstDay3xThreshold(weather[y]); 
			CGrowingSeason GS(m_nbDaysStart, m_TtypeStart, m_thresholdStart, 0, 0, 0);
			CTPeriod p = GS.GetGrowingSeason(weather[y]);
			//firstDay = p.Begin().GetJDay();
			//firstDay = weather[y].GetFir.GetFirstDayThreshold(m_nbDaysStart, m_TtypeStart, m_thresholdStart, '>').GetJDay();

			if (p.Begin().IsInit())
			{
				firstDay = p.Begin().GetJDay();
				size_t nbDay = GetNbDayLastRain(weather[y], firstDay);
				FFMC = 85;
				DMC = 2 * nbDay;
				DC = 5 * nbDay;
			}
			else
			{
				//we start the first on july
				//a very cold place, no fire problem...
				firstDay = 183;
				FFMC = 85;
				DMC = 6;
				DC = 15;
			}
		}

		//now we apply DC transfer
		if (y > 0 && lastDay >= 0)
		{
			ASSERT(y > 0);
			CTPeriod p(CJDayRef(weather[y - 1].GetTRef().GetYear(), lastDay), CJDayRef(weather[y].GetTRef().GetYear(), firstDay));
			double Rw = weather.GetStat(H_PRCP, p)[SUM];

			if (Rw < 200)
			{
				//if sum of precipitation is under 200, then we report last DC
				const double a = m_carryOverFraction;
				const double b = m_effectivenessOfWinterPrcp;

				double Qf = 800 * exp(-lastDC / 400);
				double Qs = a * Qf + b * 3.94*Rw;
				DC = max(0.0, 400 * log(800 / Qs));
			}
		}

		return firstDay;
	}


	ERMsg CFWI::Execute(const CWeatherStation& weather, CModelStatVector& output)
	{
		ASSERT(weather.IsHourly());

		ERMsg msg;
		output.clear();
		output.Init(weather.GetEntireTPeriod(CTM(m_method == ALL_HOURS_CALCULATION ? CTM::HOURLY : CTM::DAILY)), CFWIStat::NB_D_STAT, MISSING);
		
		
		bool fbpMod = false;
		bool bContinueMode = false;
		size_t firstDay = NOT_INIT;
		size_t lastDay = NOT_INIT;
		double oldFFMC = m_FFMC;
		double oldDMC = m_DMC;
		double oldDC = m_DC;

		for (size_t y = 0; y < weather.size() && msg; y++)//for all years
		{
			int year = weather.GetFirstYear() + int(y);

			if (m_bAutoSelect)//auto init parameter
			{
				if (bContinueMode)
					firstDay = 0;
				else
					firstDay = GetInitialValue(weather, y, lastDay, oldFFMC, oldDMC, oldDC);

				//compute the new last day for this year
				lastDay = GetLastDay(weather[y]).GetJDay();
			}
			else
			{
				if (m_init_values.empty())
				{
					//take default values
					firstDay = m_firstDay.GetTRef(year).GetJDay();
					lastDay = m_lastDay.GetTRef(year).GetJDay();
					oldFFMC = m_FFMC;
					oldDMC = m_DMC;
					oldDC = m_DC;
				}
				else
				{
					string ID = weather.GetLocation().m_ID + "_" + to_string(year);

					if (m_init_values.find(ID) != m_init_values.end())
					{
						ASSERT(size_t(m_init_values[ID][FWI_START_DATE]) < 366);
						ASSERT(size_t(m_init_values[ID][FWI_END_DATE]) == NOT_INIT || size_t(m_init_values[ID][FWI_END_DATE]) < 366);

						//set first day
						firstDay = CJDayRef(year, size_t(m_init_values[ID][FWI_START_DATE])).GetJDay();

						//set last day
						if (m_init_values[ID][FWI_END_DATE] != NOT_INIT)
							lastDay = CJDayRef(year, size_t(m_init_values[ID][FWI_END_DATE])).GetJDay();
						else //take default value
							lastDay = m_lastDay.GetTRef(year).GetJDay();

						oldFFMC = m_init_values[ID][FWI_FFMC];
						oldDMC = m_init_values[ID][FWI_DMC];
						oldDC = m_init_values[ID][FWI_DC];
					}
					else
					{
						msg.ajoute("location/year not found in initial values: " + ID);
						return msg;
					}

				}
			}

			if (m_method == ALL_HOURS_CALCULATION)
			{

				//CTPeriod p = weather[y].GetEntireTPeriod(CTM::HOURLY);
				CTPeriod p = weather[y].GetEntireTPeriod(CTM::DAILY);
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all day
				{
					const CWeatherDay& pday = weather.GetDay(TRef).HavePrevious() ? weather.GetDay(TRef).GetPrevious() : weather.GetDay(TRef);
					const CWeatherDay& day = weather.GetDay(TRef);

				//	const CHourlyData& hour = weather.GetHour(TRef);
					size_t jd = TRef.GetJDay();

					if (jd >= firstDay && jd <= lastDay)
					{
						double lat = weather.GetLocation().m_lat;
						size_t m = TRef.GetMonth();
						double Tnoon = day[12][H_TAIR];
						double RHnoon = day[12][H_RELH];
						double prcp = 0;// day[H_PRCP][SUM];
						for (size_t h = 13; h < 24; h++)
							if (!WEATHER::IsMissing(pday[h][H_PRCP]))
								prcp += pday[h][H_PRCP];
						for (size_t h = 0; h <= 12; h++)
							if (!WEATHER::IsMissing(day[h][H_PRCP]))
								prcp += day[h][H_PRCP];

						double DMC = GetDMC(oldDMC, m, Tnoon, RHnoon, prcp);

						// compute DC 
						//double DC = GetDC(oldDC, day, prcp);
						double DC = GetDC(oldDC, lat, m, Tnoon, prcp);

						// compute BUI from DMC and DC
						double BUI = GetBUI(DMC, DC);


						for (size_t h = 0; h < 24; h++)
						{
							const CHourlyData& hour = day[h];
							
							// compute FFMC
							//double FFMC = GetHFFMC(oldFFMC, hour);
							double FFMC = GetHFFMC(oldFFMC, hour[H_TAIR], hour[H_RELH], hour[H_WNDS], hour[H_PRCP]);

							// compute ISI
							//double ISI = GetHISI(FFMC, hour);
							double ISI = GetHISI(FFMC, hour[H_WNDS], fbpMod);

							// compute FWI from BUI ans ISI
							double FWI = GetFWI(BUI, ISI);

							// compute DSR from FWI
							double DSR = GetDSR(FWI);
							ASSERT(DSR < 200);

							//save result
							CTRef TRefh = hour.GetTRef();
							output[TRefh][CFWIStat::TMEAN_NOON] = hour[H_TAIR];
							output[TRefh][CFWIStat::RELH_NOON] = hour[H_RELH];
							output[TRefh][CFWIStat::WNDS_NOON] = hour[H_WNDS];
							output[TRefh][CFWIStat::PRCP] = hour[H_PRCP];
							output[TRefh][CFWIStat::FFMC] = FFMC;
							output[TRefh][CFWIStat::DMC] = DMC;
							output[TRefh][CFWIStat::DC] = DC;
							output[TRefh][CFWIStat::ISI] = ISI;
							output[TRefh][CFWIStat::BUI] = BUI;
							output[TRefh][CFWIStat::FWI] = FWI;
							output[TRefh][CFWIStat::DSR] = DSR;
							
							oldFFMC = FFMC;
						}

						oldDMC = DMC;
						oldDC = DC;

					}
				} //for all hours
			}
			else if (m_method == NOON_CALCULATION)
			{
				CTPeriod p = weather[y].GetEntireTPeriod(CTM::DAILY);
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all day
				{
					const CWeatherDay& pday = weather.GetDay(TRef).HavePrevious() ? weather.GetDay(TRef).GetPrevious() : weather.GetDay(TRef);
					const CWeatherDay& day = weather.GetDay(TRef);

					size_t jd = TRef.GetJDay();

					if (jd >= firstDay && jd <= lastDay)
					{
						double lat = weather.GetLocation().m_lat;
						size_t m = TRef.GetMonth();
						double Tnoon = day[12][H_TAIR];
						double RHnoon = day[12][H_RELH];
						double WSnoon = day[12][H_WNDS];
						double prcp = 0;// day[H_PRCP][SUM];
						for (size_t h = 13; h < 24; h++)
							if (!WEATHER::IsMissing(pday[h][H_PRCP]))
								prcp += pday[h][H_PRCP];
						for (size_t h = 0; h <= 12; h++)
							if (!WEATHER::IsMissing(day[h][H_PRCP]))
								prcp += day[h][H_PRCP];

						// compute DMC
						//double DMC = GetDMC(oldDMC, day, prcp);
						double DMC = GetDMC(oldDMC, m, Tnoon, RHnoon, prcp);

						// compute DC 
						//double DC = GetDC(oldDC, day, prcp);
						double DC = GetDC(oldDC, lat, m, Tnoon, prcp);

						// compute BUI from DMC and DC
						double BUI = GetBUI(DMC, DC);


						// compute FFMC
						//double FFMC = GetFFMC(oldFFMC, day, prcp);
						double FFMC = GetFFMC(oldFFMC, Tnoon, RHnoon, WSnoon, prcp);

						// compute ISI
						//double ISI = GetISI(FFMC, day);
						double ISI = GetISI(FFMC, WSnoon, fbpMod);
						
						// compute FWI from BUI ans ISI
						double FWI = GetFWI(BUI, ISI);

						// compute DSR from FWI
						double DSR = GetDSR(FWI);
						ASSERT(DSR < 200);

						//save result
						output[TRef][CFWIStat::TMEAN_NOON] = Tnoon;
						output[TRef][CFWIStat::RELH_NOON] = RHnoon;
						output[TRef][CFWIStat::WNDS_NOON] = WSnoon;
						output[TRef][CFWIStat::PRCP] = prcp;
						output[TRef][CFWIStat::FFMC] = FFMC;
						output[TRef][CFWIStat::DMC] = DMC;
						output[TRef][CFWIStat::DC] = DC;
						output[TRef][CFWIStat::ISI] = ISI;
						output[TRef][CFWIStat::BUI] = BUI;
						output[TRef][CFWIStat::FWI] = FWI;
						output[TRef][CFWIStat::DSR] = DSR;

						oldFFMC = FFMC;
						oldDMC = DMC;
						oldDC = DC;
					}
				}
			}//for all day
		}//for all year 


		return msg;
	}


	//**************************************************************
	void CFWIStat::Covert2D(const CModelStatVector& result, CModelStatVector& resultD)
	{
		_ASSERTE(result.GetNbStat() == CFWIStat::NB_D_STAT);
		CStatistic::SetVMiss(CFWI::MISSING);

		CTPeriod p = result.GetTPeriod();
		if (p.GetTM().Type() == CTM::DAILY)
		{
			resultD = result;
		}
		else
		{
			CTStatMatrix tmp(result, CTM::DAILY);
			resultD.Init(tmp.m_period, CFWIStat::NB_D_STAT, CFWI::MISSING);


			for (CTRef d = tmp.m_period.Begin(); d <= tmp.m_period.End(); d++)
			{
				for (size_t v = 0; v < resultD.GetNbStat(); v++)
					resultD[d][v] = tmp[d][v][v == PRCP ? SUM : MEAN];
			}
		}
	}

	void CFWIStat::Covert2M(const CModelStatVector& resultD, CModelStatVector& resultM)
	{
		_ASSERTE(resultD.GetNbStat() == CFWIStat::NB_D_STAT);
		CStatistic::SetVMiss(CFWI::MISSING);

		CTRef firstDate = resultD.GetFirstTRef();
		CTRef lastDate = resultD.GetLastTRef();

		int nbYear = lastDate.GetYear() - firstDate.GetYear() + 1;
		resultM.SetFirstTRef(CTRef(firstDate.GetYear(), 0));
		resultM.resize(nbYear * 12);

		for (CTRef d = firstDate; d <= lastDate; )
		{
			int year = d.GetYear();//firstDate.GetYear() + y;

			for (size_t m = 0; m < 12; m++)
			{
				CStatistic stat[CFWIStat::NB_D_STAT];
				while (d <= lastDate &&
					d.GetYear() == year &&
					d.GetMonth() == m)
				{
					if (resultD[d][FWI] > CFWI::MISSING)
					{
						for (int v = 0; v < resultD.GetNbStat(); v++)
							stat[v] += resultD[d][v];
					}

					d++;
				}

				CTRef ref(year, m);

				resultM[ref][CFWIStat::NUM_VALUES] = stat[0][NB_VALUE];
				for (int v = 0; v < resultD.GetNbStat(); v++)
				{
					//short s = (v==CFWIStat::PRCP)?SUM:MEAN;
					if (v == CFWIStat::PRCP)
					{
						CTPeriod p(CTRef(year, m, FIRST_DAY), CTRef(year, m, LAST_DAY));
						resultM[ref][CFWIStat::TMEAN_NOON + v] = stat[v][SUM];
						resultM[ref][CFWIStat::TMEAN_MIN + v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, true);
						resultM[ref][CFWIStat::TMEAN_MAX + v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, false);
					}
					else
					{
						resultM[ref][CFWIStat::TMEAN_NOON + v] = stat[v][MEAN];
						resultM[ref][CFWIStat::TMEAN_MIN + v] = stat[v][LOWEST];
						resultM[ref][CFWIStat::TMEAN_MAX + v] = stat[v][HIGHEST];
					}
				}
			}
		}
	}

	void CFWIStat::Covert2A(const CModelStatVector& resultD, CModelStatVector& resultA)
	{
		_ASSERTE(resultD.GetNbStat() == CFWIStat::NB_D_STAT);

		CTRef firstDate = resultD.GetFirstTRef();
		CTRef lastDate = resultD.GetLastTRef();

		int nbYear = lastDate.GetYear() - firstDate.GetYear() + 1;
		resultA.SetFirstTRef(CTRef((short)firstDate.GetYear()));
		resultA.resize(nbYear, CFWI::MISSING);

		for (CTRef d = firstDate; d <= lastDate; )
		{
			short year = d.GetYear();
			CTRef firstDay;
			CTRef lastDay;

			CStatistic stat[CFWIStat::NB_D_STAT];
			while (d <= lastDate &&
				d.GetYear() == year)
			{
				//_ASSERTE( resultD[d][FWI] > -9999 );
				if (resultD[d][FWI] > CFWI::MISSING)
				{
					if (!firstDay.IsInit())
						firstDay = d;

					for (int v = 0; v < resultD.GetNbStat(); v++)
						stat[v] += resultD[d][v];

					lastDay = d;
				}

				d++;
			}


			CTRef ref((short)year);
			resultA[ref][CFWIStat::NUM_VALUES] = stat[0][NB_VALUE];
			resultA[ref][CFWIStat::FIRST_FWI_DAY] = firstDay.GetJDay() + 1;
			resultA[ref][CFWIStat::LAST_FWI_DAY] = lastDay.GetJDay() + 1;

			for (size_t v = 0; v < resultD.GetNbStat(); v++)
			{
				if (v == CFWIStat::PRCP)
				{
					CTPeriod p(CTRef(year, FIRST_MONTH, FIRST_DAY), CTRef(year, LAST_MONTH, LAST_DAY));
					resultA[ref][CFWIStat::TMEAN_NOON + v] = stat[v][SUM];
					resultA[ref][CFWIStat::TMEAN_MIN + v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, true);
					resultA[ref][CFWIStat::TMEAN_MAX + v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, false);
				}
				else
				{
					resultA[ref][CFWIStat::TMEAN_NOON + v] = stat[v][MEAN];
					resultA[ref][CFWIStat::TMEAN_MIN + v] = stat[v][LOWEST];
					resultA[ref][CFWIStat::TMEAN_MAX + v] = stat[v][HIGHEST];
				}
			}
		}
	}

}//WBSF

