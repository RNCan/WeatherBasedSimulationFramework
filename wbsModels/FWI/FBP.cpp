//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
//   An internal function used to setup the calculation of the Fire Behavior
//   Prediction(FBP) system.This function moves the logic of the FBP system
//   equations into a C++ interface.
//
//
// Args:
//   input : Data frame of required and optional information needed to
//           calculate FBP function.View the arguments section of the fbp
//           manual(fbp.Rd) under "input" for the full listing of the
//           required and optional inputs.
//
// Returns : all FBP outputs
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 13/10/2020   Rémi Saint-Amant	Code from R source: https://rdrr.io/cran/cffdrs
//**********************************************************************
#include <math.h>
#include <assert.h>
#include "FBP.h"
#include "FWI.h"

//******************************************

using namespace std;

namespace WBSF
{




	static const double pi = 3.141592653589793;
	const char* CFBP::FUEL_NAMES[NB_FUEL_TYPE] = { "C1", "C2", "C3", "C4", "C5", "C6", "C7","D1", "M1", "M2", "M3", "M4", "S1", "S2", "S3", "O1A","O1B" };
	const double CFBP::CBHs[NB_FUEL_TYPE] = { 2, 3, 8, 4, 18, 7, 10, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0 };
	const double CFBP::CFLs[NB_FUEL_TYPE] = { 0.75, 0.8, 1.15, 1.2, 1.2, 1.8, 0.5, 0, 0.8, 0.8, 0.8, 0.8, 0, 0, 0, 0, 0 };
	const double CFBP::a[NB_FUEL_TYPE] = { 90, 110, 110, 110, 30, 30, 45, 30, 0, 0, 120, 100, 75, 40, 55, 190, 250 };
	const double CFBP::b[NB_FUEL_TYPE] = { 0.0649, 0.0282, 0.0444, 0.0293, 0.0697, 0.0800, 0.0305, 0.0232, 0, 0,0.0572, 0.0404, 0.0297, 0.0438, 0.0829, 0.0310, 0.0350 };
	const double CFBP::c0[NB_FUEL_TYPE] = { 4.5, 1.5, 3.0, 1.5, 4.0, 3.0, 2.0, 1.6, 0, 0, 1.4, 1.48, 1.3, 1.7,3.2, 1.4, 1.7 };
	const double CFBP::PDF100 = 100;//100 % Dead Balsam Fir
	const double CFBP::NoBUI = -1;

	TFuel CFBP::GetFuelType(const string& fuel_type)
	{
		TFuel fuel = FUEL_UNKNOWNS;
		auto it = find(begin(FUEL_NAMES), end(FUEL_NAMES), fuel_type);
		if (it != end(FUEL_NAMES))
			fuel = TFuel(std::distance(begin(FUEL_NAMES), it));

		return fuel;
	}


	//**************************************************************************************************
	// Description :
	//   Fire Behavior Prediction System calculations.This is the primary
	//  function for calculating FBP for a single timestep.Not all equations are
	//  calculated within this function, but have been broken down further.
	//
	//
	//Args:
	//  input : CFBPInput
	//
	//Returns : 
	//  CFBPOutput : all FBP outputs
	//
	//**************************************************************************************************
	void CFBP::Execute(CFBPInput in, CFBPOutput& out)
	{

		if (in.fuel_type == FUEL_UNKNOWNS)
			in.fuel_type = FUEL_C2;
		if (in.FFMC <= -999)
			in.FFMC = 90;
		if (in.BUI <= -999)
			in.BUI = 60;
		if (in.ISI <= -999)
			in.ISI = 0;
		if (in.WD <= -999)
			in.WD = 0;
		if (in.FMC <= -999)
			in.FMC = 0;
		if (in.ELV <= -999)
			in.ELV = 0;
		if (in.SD <= -999)
			in.SD = 0;
		if (in.SH <= -999)
			in.SH = 0;
		if (in.D0 > 366)
			in.D0 = 0;
		if (in.hr <= -999)
			in.hr = 1;
		if (in.PC <= -999)
			in.PC = 50;
		if (in.PDF <= -999)
			in.PDF = 35;
		if (in.GFL <= -999)
			in.GFL = 0.35;
		if (in.cc <= -999)
			in.cc = 80;
		if (in.theta <= -999)
			in.theta = 0;
		if (in.CBH <= -999)
			in.CBH = 0;
		if (in.CFL <= -999)
			in.CFL = 0;
		if (in.ISI <= -999)
			in.ISI = 0;
		if (in.GS <= -999)
			in.GS = 0;
		if (in.ASPECT <= -999)
			in.ASPECT = 0;


		assert(in.WD >= 0 && in.WS <= 360);
		assert(in.theta >= 0 && in.theta <= 360);
		assert(in.ASPECT >= 0 && in.ASPECT <= 360);
		assert(in.DJ >= 0 && in.DJ < 366);
		assert(in.D0 >= 0 && in.D0 < 366);
		assert(in.ELV >= -100 && in.ELV < 10000);
		assert(in.BUIeff == 0 || in.BUIeff == 1);
		assert(in.hr >= 0 && in.hr <= 366 * 24);
		assert(in.FFMC >= 0 && in.FFMC <= 101);
		assert(in.ISI >= 0 && in.ISI <= 300);
		assert(in.ISI >= 0 && in.ISI <= 1000);
		assert(in.WS >= 0 && in.WS <= 300);
		assert(in.GS >= 0 && in.GS <= 200);
		assert(in.PC >= 0 && in.PC <= 100);
		assert(in.PDF >= 0 && in.PDF <= 100);
		assert(in.cc >= 0 && in.cc <= 100);
		assert(in.GFL >= 0 && in.GFL <= 100);
		assert(in.SD >= 0 && in.SD <= 1e+05);
		assert(in.SH >= 0 && in.SH <= 100);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//                        START
		//Corrections
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//Convert WD from degress to radians
		in.WD = in.WD * pi / 180;
		//Convert Theta from degress to radians
		in.theta = in.theta * pi / 180;
		//Convert Aspect from degress to radians
		in.ASPECT = in.ASPECT * pi / 180;

		//Convert hours to minutes
		in.hr = in.hr * 60;
		//Corrections to reorient Wind Azimuth(WAZ) and Uphill slode azimuth(SAZ)
		double WAZ = in.WD + pi;
		WAZ = (WAZ > 2 * pi) ? WAZ - 2 * pi : WAZ;
		double SAZ = in.ASPECT + pi;
		SAZ = (SAZ > 2 * pi) ? SAZ - 2 * pi : SAZ;

		//Any negative longitudes(western hemisphere) are translated to positive longitudes
		in.LONG = (in.LONG < 0) ? -in.LONG : in.LONG;
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//                        END
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//                        START
		//Initializing variables
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



		if (in.CBH <= 0 || in.CBH > 50)
		{
			if (in.fuel_type == FUEL_C6 && in.SD > 0 && in.SH > 0)
				in.CBH = -11.2 + 1.06 * in.SH + 0.0017 *in.SD;
			else
				in.CBH = CBHs[in.fuel_type];
		}

		//CBH = ifelse(CBH < 0, 1e-07, CBH);
		if (in.CBH < 0)
			in.CBH = 1e-07;
		//in.CBH = max(1e-07, in.CBH);


		if (in.CFL <= 0 || in.CFL > 2)
			in.CFL = CFLs[in.fuel_type];



		static const TFuel FMC_FUEL[] = { FUEL_D1, FUEL_S1, FUEL_S2, FUEL_S3, FUEL_O1A, FUEL_O1B };
		if (find(begin(FMC_FUEL), end(FMC_FUEL), in.fuel_type) != end(FMC_FUEL))
		{
			in.FMC = 0;
		}
		else
		{
			if (in.FMC <= 0 || in.FMC > 120)
				in.FMC = FMCcalc(in.LAT, in.LONG, in.ELV, in.DJ, in.D0);
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//                        END
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//Calculate Surface fuel consumption(SFC)
		out.SFC = SFCcalc(in.fuel_type, in.FFMC, in.BUI, in.PC, in.GFL);
		//Disable BUI Effect if necessary
		if (in.BUIeff != 1)
			in.BUI = 0;

		//Calculate the net effective windspeed(WSV)
		double WSV0 = Slopecalc(in.fuel_type, in.FFMC, in.BUI, in.WS, WAZ, in.GS, SAZ, in.FMC, out.SFC, in.PC, in.PDF, in.cc, in.CBH, in.ISI, "WSV");
		out.WSV = (in.GS > 0 && in.FFMC > 0) ? WSV0 : in.WS;

		//Calculate the net effective wind direction(RAZ)
		double RAZ0 = Slopecalc(in.fuel_type, in.FFMC, in.BUI, in.WS, WAZ, in.GS, SAZ, in.FMC, out.SFC, in.PC, in.PDF, in.cc, in.CBH, in.ISI, "RAZ");
		out.RAZ = (in.GS > 0 && in.FFMC > 0) ? RAZ0 : WAZ;

		//Calculate or keep Initial Spread Index(ISI)
		if (in.ISI <= 0)
			in.ISI = CFWI::GetISI(in.FFMC, out.WSV, TRUE);

		//Calculate the Rate of Spread(ROS), C6 has different calculations
		out.ROS = (in.fuel_type == FUEL_C6) ? C6calc(in.fuel_type, in.ISI, in.BUI, in.FMC, out.SFC, in.CBH, /*-999, -999, -999, */"ROS") :
			ROScalc(in.fuel_type, in.ISI, in.BUI, in.FMC, out.SFC, in.PC, in.PDF, in.cc, in.CBH);

		//Calculate Crown Fraction Burned(CFB), C6 has different calculations
		out.CFB = (in.fuel_type == FUEL_C6) ? C6calc(in.fuel_type, in.ISI, in.BUI, in.FMC, out.SFC, in.CBH, /*-999, -999, -999, */"CFB") :
			(in.CFL > 0) ? CFBcalc(in.fuel_type, in.FMC, out.SFC, out.ROS, in.CBH) : 0;

		//Calculate Total Fuel Consumption(TFC)
		out.TFC = TFCcalc(in.fuel_type, in.CFL, out.CFB, out.SFC, in.PC, in.PDF);
		//Calculate Head Fire Intensity(HFI)
		out.HFI = FIcalc(out.TFC, out.ROS);
		//Adjust Crown Fraction Burned
		if (in.hr < 0)
			out.CFB = -out.CFB;

		//convert radian to degree RAZ
		out.RAZ = out.RAZ * 180 / pi;
		assert(out.RAZ >= 0 && out.RAZ <= 360);

		if (out.RAZ == 360)
			out.RAZ = 0;

		//Calculate Fire Type(S = Surface, C = Crowning, I = Intermittent Crowning)
		out.FD = (out.CFB < 0.1) ? 'S' : (out.CFB >= 0.9) ? 'C' : 'I';

		//Calculate Crown Fuel Consumption(CFC)
		out.CFC = TFCcalc(in.fuel_type, in.CFL, out.CFB, out.SFC, in.PC, in.PDF, "CFC");
		//Calculate the Secondary Outputs

		//Eq. 39 (FCFDG 1992) Calculate Spread Factor(GS is group slope)
		out.SF = (in.GS >= 70) ? 10 : exp(3.533 * pow(in.GS / 100, 1.2));
		//Calculate Critical Surface Intensity
		out.CSI = CFBcalc(in.fuel_type, in.FMC, out.SFC, out.ROS, in.CBH, "CSI");
		//Calculate Surface fire rate of spread(m / min)
		out.RSO = CFBcalc(in.fuel_type, in.FMC, out.SFC, out.ROS, in.CBH, "RSO");
		//Calculate The Buildup Effect
		out.BE = BEcalc(in.fuel_type, in.BUI);
		//Calculate length to breadth ratio
		out.LB = LBcalc(in.fuel_type, out.WSV);
		out.LBt = (in.accel == AC_LINE) ? out.LB : LBtcalc(in.fuel_type, out.LB, in.hr, out.CFB);
		//Calculate Back fire rate of spread(BROS)
		out.BROS = BROScalc(in.fuel_type, in.FFMC, in.BUI, out.WSV, in.FMC, out.SFC, in.PC, in.PDF, in.cc, in.CBH);
		//Calculate Flank fire rate of spread(FROS)
		out.FROS = FROScalc(out.ROS, out.BROS, out.LB);
		//Calculate the eccentricity
		double E = sqrt(1 - 1 / out.LB / out.LB);
		//Calculate the rate of spread towards angle theta(TROS)
		out.TROS = out.ROS * (1 - E) / (1 - E * cos(in.theta - out.RAZ));
		//Calculate rate of spread at time t for Flank, Back of fire and at angle theta.
		double ROSt = (in.accel == AC_LINE) ? out.ROS : ROStcalc(in.fuel_type, out.ROS, in.hr, out.CFB);
		out.BROSt = (in.accel == AC_LINE) ? out.BROS : ROStcalc(in.fuel_type, out.BROS, in.hr, out.CFB);
		out.FROSt = (in.accel == AC_LINE) ? out.FROS : FROScalc(ROSt, out.BROSt, out.LBt);
		//Calculate rate of spread towards angle theta at time t(TROSt)
		out.TROSt = (in.accel == AC_LINE) ? out.TROS :
			ROSt * (1 - sqrt(1 - 1 / out.LBt / out.LBt)) /
			(1 - sqrt(1 - 1 / out.LBt / out.LBt) * cos(in.theta - out.RAZ));
		//Calculate Crown Fraction Burned for Flank, Back of fire and at angle theta.
		out.FCFB = (in.CFL == 0) ? 0 :
			(in.fuel_type == FUEL_C6) ? 0 : CFBcalc(in.fuel_type, in.FMC, out.SFC, out.FROS, in.CBH);
		out.BCFB = (in.CFL == 0) ? 0 :
			(in.fuel_type == FUEL_C6) ? 0 : CFBcalc(in.fuel_type, in.FMC, out.SFC, out.BROS, in.CBH);
		out.TCFB = (in.CFL == 0) ? 0 :
			(in.fuel_type == FUEL_C6) ? 0 : CFBcalc(in.fuel_type, in.FMC, out.SFC, out.TROS, in.CBH);
		//Calculate Total fuel consumption for the Flank fire, Back fire and at
		// angle theta
		out.FTFC = TFCcalc(in.fuel_type, in.CFL, out.FCFB, out.SFC, in.PC, in.PDF);
		out.BTFC = TFCcalc(in.fuel_type, in.CFL, out.BCFB, out.SFC, in.PC, in.PDF);
		out.TTFC = TFCcalc(in.fuel_type, in.CFL, out.TCFB, out.SFC, in.PC, in.PDF);
		//Calculate the Fire Intensity at the Flank, Back and at angle theta fire
		out.FFI = FIcalc(out.FTFC, out.FROS);
		out.BFI = FIcalc(out.BTFC, out.BROS);
		out.TFI = FIcalc(out.TTFC, out.TROS);
		//Calculate Rate of spread at time t for the Head, Flank, Back of fire and
		// at angle theta.
		out.HROSt = (in.hr < 0) ? -ROSt : ROSt;

		if (in.hr < 0)
		{
			out.FROSt = -out.FROSt;
			out.BROSt = -out.BROSt;
			out.TROSt = -out.TROSt;
		}

		//Calculate the elapsed time to crown fire initiation for Head, Flank, Back
		//fire and at angle theta.The(a//variable is a constant for Head, Flank,
			//Back and at angle theta used in the *TI equations)
		double a1 = 0.115 - (18.8 * pow(out.CFB, 2.5) * exp(-8 * out.CFB));
		out.TI = log((1 - out.RSO / out.ROS > 0) ? 1 - out.RSO / out.ROS : 1) / (-a1);
		double a2 = 0.115 - (18.8 * pow(out.FCFB, 2.5) * exp(-8 * out.FCFB));
		out.FTI = log((1 - out.RSO / out.FROS > 0) ? 1 - out.RSO / out.FROS : 1) / (-a2);
		double a3 = 0.115 - (18.8 * pow(out.BCFB, 2.5) * exp(-8 * out.BCFB));
		out.BTI = log((1 - out.RSO / out.BROS > 0) ? 1 - out.RSO / out.BROS : 1) / (-a3);
		double a4 = 0.115 - (18.8 * pow(out.TCFB, 2.5) * exp(-8 * out.TCFB));
		out.TTI = log((1 - out.RSO / out.TROS > 0) ? 1 - out.RSO / out.TROS : 1) / (-a4);

		//Fire spread distance for Head, Back, and Flank of fire
		out.DH = (in.accel == 1) ? DISTtcalc(in.fuel_type, out.ROS, in.hr, out.CFB) : out.ROS * in.hr;
		out.DB = (in.accel == 1) ? DISTtcalc(in.fuel_type, out.BROS, in.hr, out.CFB) : out.BROS * in.hr;
		out.DF = (in.accel == 1) ? (out.DH + out.DB) / (out.LBt * 2) : (out.DH + out.DB) / (out.LB * 2);


		//copy from input
		out.ISI = in.ISI;
		out.FFMC = in.FFMC;
		out.FMC = in.FMC;
		out.D0 = in.D0;
	}

	//**************************************************************************************************
	// Description:
	//   Computes the Surface Fuel Consumption by Fuel Type.
	//   All variables names are laid out in the same manner as FCFDG(1992) or
	//   Wotton et.al(2009)
	//   Forestry Canada Fire Danger Group(FCFDG) (1992). "Development and 
	//   Structure of the Canadian Forest Fire Behavior Prediction System." 
	//   Technical Report ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//        BUI : Buildup Index
	//       FFMC : Fine Fuel Moisture Code
	//         PC : Percent Conifer(%)
	//        GFL : Grass Fuel Load(kg / m ^ 2)
	// Returns :
	//        SFC : Surface Fuel Consumption(kg / m ^ 2)
	//
	//**************************************************************************************************
	double CFBP::SFCcalc(size_t FUELTYPE, double FFMC, double BUI, double PC, double GFL)
	{
		double SFC = -999;

		if (FUELTYPE == FUEL_C1)
		{
			// for the C1 fuel type SFC calculation
			//Eqs. 9a, 9b(Wotton et.al. 2009) - Solving the lower bound of FFMC value
			SFC = (FFMC > 84) ?
				0.75 + 0.75 * pow(1 - exp(-0.23 * (FFMC - 84)), 0.5) :
				0.75 - 0.75 * pow(1 - exp(-0.23 * (84 - FFMC)), 0.5);
		}
		else if (FUELTYPE == FUEL_C2 || FUELTYPE == FUEL_M3 || FUELTYPE == FUEL_M4)
		{
			//Eq. 10 (FCFDG 1992) - C2, M3, and M4 Fuel Types
			SFC = 5.0 * (1 - exp(-0.0115 * BUI));
		}
		else if (FUELTYPE == FUEL_C3 || FUELTYPE == FUEL_C4)
		{
			//Eq. 11 (FCFDG 1992) - C3, C4 Fuel Types
			SFC = 5.0 * pow(1 - exp(-0.0164 * BUI), 2.24);
		}
		else if (FUELTYPE == FUEL_C5 || FUELTYPE == FUEL_C6)
		{
			//Eq. 12 (FCFDG 1992) - C5, C6 Fuel Types
			SFC = 5.0 * pow(1 - exp(-0.0149 * BUI), 2.48);
		}
		else if (FUELTYPE == FUEL_C7)
		{
			//Eqs. 13, 14, 15 (FCFDG 1992) - C7 Fuel Types
			SFC = (FFMC > 70) ? 2 * (1 - exp(-0.104 * (FFMC - 70))) : 0;
			SFC += 1.5 * (1 - exp(-0.0201 * BUI));
			//ifelse(FFMC > 70, 2 * (1 - exp(-0.104 * (FFMC - 70))), 
			//	0) + 1.5 * (1 - exp(-0.0201 * BUI))
		}
		else if (FUELTYPE == FUEL_D1)
		{
			//Eq. 16 (FCFDG 1992) - D1 Fuel Type
			SFC = 1.5 * (1 - exp(-0.0183 * BUI));
		}

		else if (FUELTYPE == FUEL_M1 || FUELTYPE == FUEL_M2)
		{
			//Eq. 17 (FCFDG 1992) - M1 and M2 Fuel Types
			SFC = PC / 100 * (5.0 * (1 - exp(-0.0115 * BUI))) +
				((100 - PC) / 100 * (1.5 * (1 - exp(-0.0183 * BUI))));
		}
		else if (FUELTYPE == FUEL_O1A || FUELTYPE == FUEL_O1B)
		{
			//Eq. 18 (FCFDG 1992) - Grass Fuel Types
			SFC = GFL;
		}
		else if (FUELTYPE == FUEL_S1)
		{
			//Eq. 19, 20, 25 (FCFDG 1992) - S1 Fuel Type
			SFC = 4.0 * (1 - exp(-0.025 * BUI)) + 4.0 * (1 - exp(-0.034 * BUI));
		}
		else if (FUELTYPE == FUEL_S2)
		{
			//Eq. 21, 22, 25 (FCFDG 1992) - S2 Fuel Type
			SFC = 10.0 * (1 - exp(-0.013 * BUI)) + 6.0 * (1 - exp(-0.060 * BUI));
		}
		else if (FUELTYPE == FUEL_S3)
		{
			//Eq. 23, 24, 25 (FCFDG 1992) - S3 Fuel Type
			SFC = 12.0 * (1 - exp(-0.0166 * BUI)) + 20.0 * (1 - exp(-0.0210 * BUI));
		}

		//Constrain SFC value
		SFC = max(0.000001, SFC);

		return SFC;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description :
	//   Calculate the net effective windspeed(WSV), the net effective wind direction(RAZ)
	//
	//   All variables names are laid out in the same manner as FCFDG(1992) and
	//   Wotton(2009).
	//
	//   
	//   Forestry Canada Fire Danger Group(FCFDG) (1992). "Development and 
	//   Structure of the Canadian Forest Fire Behavior Prediction System." 
	//   Technical Report ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//       FFMC : Fine Fuel Moisture Code
	//        BUI : The Buildup Index value
	//         WS : Windspeed(km / h)
	//        WAZ : Wind Azimuth
	//         GS : Ground Slope(%)
	//        SAZ : Slope Azimuth
	//        FMC : Foliar Moisture Content
	//        SFC : Surface Fuel Consumption(kg / m ^ 2)
	//         PC : Percent Conifer(%)
	//        PDF : Percent Dead Balsam Fir(%)
	//         CC : Constant
	//        CBH : Crown Base Height(m)
	//        ISI : Initial Spread Index
	//     output : Type of variable to output(RAZ / WSV, default = RAZ)
	// Returns :
	//   BE : The Buildup Effect
	//
	// output options include : RAZ and WSV
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::Slopecalc(size_t FUELTYPE, double FFMC, double BUI, double WS, double WAZ, double GS, double SAZ, double FMC, double SFC, double PC, double PDF, double CC, double CBH, double ISI, string output)
	{
		assert(output == "RAZ" || output == "WSV");


		//Eq. 39 (FCFDG 1992) - Calculate Spread Factor
		double SF = (GS >= 70) ? 10 : exp(3.533 * pow(GS / 100, 1.2));
		//ISI with 0 wind on level grounds
		double ISZ = CFWI::GetISI(FFMC, 0);


		//initialize some local vars
		double ISF = -99;

		static const TFuel ISF_FUEL[] = { FUEL_C1, FUEL_C2, FUEL_C3, FUEL_C4, FUEL_C5, FUEL_C6, FUEL_C7, FUEL_D1, FUEL_S1, FUEL_S2, FUEL_S3 };
		if (find(begin(ISF_FUEL), end(ISF_FUEL), FUELTYPE) != end(ISF_FUEL))
		{
			//Surface spread rate with 0 wind on level ground
			double RSZ = ROScalc(FUELTYPE, ISZ, NoBUI, FMC, SFC, PC, PDF, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope
			double RSF = RSZ * SF;

			//Eqs. 41a, 41b(Wotton 2009) - Calculate the slope equivalend ISI
			ISF = ((1 - pow(RSF / a[FUELTYPE], 1 / c0[FUELTYPE])) >= 0.01) ?
				log(1 - pow(RSF / a[FUELTYPE], 1 / c0[FUELTYPE])) / (-b[FUELTYPE]) :
				ISF = log(0.01) / (-b[FUELTYPE]);
		}
		else if (FUELTYPE == FUEL_M1 || FUELTYPE == FUEL_M2)
		{
			//When calculating the M1 / M2 types, we are going to calculate for both C2
			// and D1 types, and combine
			//Surface spread rate with 0 wind on level ground

			double RSZ = ROScalc(FUEL_C2, ISZ, NoBUI, FMC, SFC, PC, PDF, CC, CBH);


			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for C2
			double RSF_C2 = RSZ * SF;
			RSZ = ROScalc(FUEL_D1, ISZ, NoBUI, FMC, SFC, PC, PDF, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for D1
			double RSF_D1 = RSZ * SF;

			double RSF0 = 1 - pow(RSF_C2 / a[FUEL_C2], 1 / c0[FUEL_C2]);


			double ISF_C2 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_C2 / a[FUEL_C2], 1 / c0[FUEL_C2])) / (-b[FUEL_C2]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI
				ISF_C2 = log(0.01) / (-b[FUEL_C2]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI


			RSF0 = 1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1]);

			double ISF_D1 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1])) / (-b[FUEL_D1]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI
				ISF_D1 = log(0.01) / (-b[FUEL_D1]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI

			//Eq. 42a(Wotton 2009) - Calculate weighted average for the M1 / M2 types
			ISF = PC / 100 * ISF_C2 + (1 - PC / 100) * ISF_D1;
		}
		else if (FUELTYPE == FUEL_M3)
		{
			//Surface spread rate with 0 wind on level ground
			double RSZ = ROScalc(FUEL_M3, ISZ, NoBUI, FMC, SFC, PC, PDF100, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for M3
			double RSF_M3 = RSZ * SF;

			//Surface spread rate with 0 wind on level ground, using D1
			RSZ = ROScalc(FUEL_D1, ISZ, NoBUI, FMC, SFC, PC, PDF100, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for M3
			double RSF_D1 = RSZ * SF;

			double RSF0 = 1 - pow(RSF_M3 / a[FUEL_M3], 1 / c0[FUEL_M3]);

			double ISF_M3 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_M3 / a[FUEL_M3], 1 / c0[FUEL_M3])) / (-b[FUEL_M3]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI
				ISF_M3 = log(0.01) / (-b[FUEL_M3]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for D1
			RSF0 = 1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1]);

			double ISF_D1 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1])) / (-b[FUEL_D1]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI
				ISF_D1 = log(0.01) / (-b[FUEL_D1]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI

			//Eq. 42b(Wotton 2009) - Calculate weighted average for the M3 type
			ISF = PDF / 100 * ISF_M3 + (1 - PDF / 100) * ISF_D1;
		}
		else if (FUELTYPE == FUEL_M4)
		{
			//Surface spread rate with 0 wind on level ground, using M4
			double RSZ = ROScalc(FUEL_M4, ISZ, NoBUI, FMC, SFC, PC, PDF100, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for M4
			double RSF_M4 = RSZ * SF;

			//Surface spread rate with 0 wind on level ground, using M4
			RSZ = ROScalc(FUEL_D1, ISZ, NoBUI, FMC, SFC, PC, PDF100, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for D1
			double RSF_D1 = RSZ * SF;

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for D1
			double RSF0 = 1 - pow(RSF_M4 / a[FUEL_M4], 1 / c0[FUEL_M4]);


			double ISF_M4 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_M4 / a[FUEL_M4], 1 / c0[FUEL_M4])) / (-b[FUEL_M4]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI
				ISF_M4 = log(0.01) / (-b[FUEL_M4]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope for D1
			RSF0 = 1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1]);

			double ISF_D1 = (RSF0 >= 0.01) ?
				log(1 - pow(RSF_D1 / a[FUEL_D1], 1 / c0[FUEL_D1])) / (-b[FUEL_D1]) ://Eq. 41a(Wotton 2009) - Calculate the slope equivalent ISI(D1)
				ISF_D1 = log(0.01) / (-b[FUEL_D1]);//Eq. 41b(Wotton 2009) - Calculate the slope equivalent ISI(D1)

			//Eq. 42c(Wotton 2009) - Calculate weighted average for the M4 type
			ISF = PDF / 100 * ISF_M4 + (1 - PDF / 100.) *ISF_D1;
		}
		else if (FUELTYPE == FUEL_O1A || FUELTYPE == FUEL_O1B)
		{
			//Surface spread rate with 0 wind on level ground
			double RSZ = ROScalc(FUELTYPE, ISZ, NoBUI, FMC, SFC, PC, PDF, CC, CBH);

			//Eq. 40 (FCFDG 1992) - Surface spread rate with 0 wind upslope
			double RSF = RSZ * SF;

			//Eqs. 35a, 35b(Wotton 2009) - Curing Factor pivoting around % 58.8
			double CF = (CC < 58.8) ? 0.005 * (exp(0.061 * CC) - 1) : 0.176 + 0.02 * (CC - 58.8);

			//Eqs. 43a, 43b(Wotton 2009) - slope equivilent ISI for Grass
			ISF = ((1 - pow(RSF / (CF * a[FUELTYPE]), 1 / c0[FUELTYPE])) >= 0.01 ?
				log(1 - pow(RSF / (CF * a[FUELTYPE]), 1 / c0[FUELTYPE])) / (-b[FUELTYPE]) :
				log(0.01) / (-b[FUELTYPE]));
		}

		//Eq. 46 (FCFDG 1992)
		double m = 147.2 * (101 - FFMC) / (59.5 + FFMC);
		//Eq. 45 (FCFDG 1992) - FFMC function from the ISI equation
		double fF = 91.9 * exp(-.1386 * m) * (1 + pow(m, 5.31) / 4.93e7);
		//Eqs. 44a, 44d(Wotton 2009) - Slope equivalent wind speed
		double WSE = 1 / 0.05039 * log(ISF / (0.208 * fF));
		//Eqs. 44b, 44e(Wotton 2009) - Slope equivalent wind speed
		if (WSE > 40 && ISF < (0.999 * 2.496 * fF))
			WSE = 28 - (1 / 0.0818 * log(1 - ISF / (2.496 * fF)));

		//Eqs. 44c(Wotton 2009) - Slope equivalent wind speed
		if (WSE > 40 && ISF >= (0.999 * 2.496 * fF))
			WSE = 112.45;

		//Eq. 47 (FCFDG 1992) - resultant vector magnitude in the x - direction
		double WSX = WS * sin(WAZ) + WSE * sin(SAZ);
		//Eq. 48 (FCFDG 1992) - resultant vector magnitude in the y - direction
		double WSY = WS * cos(WAZ) + WSE * cos(SAZ);
		//Eq. 49 (FCFDG 1992) - the net effective wind speed
		double WSV = sqrt(WSX * WSX + WSY * WSY);
		//stop execution here and return WSV if requested
		if (output == "WSV")
			return WSV;

		//Eq. 50 (FCFDG 1992) - the net effective wind direction(radians)
		double RAZ = acos(WSY / WSV);
		//Eq. 51 (FCFDG 1992) - convert possible negative RAZ into more understandable directions
		if (WSX < 0)
			RAZ = 2 * pi - RAZ;

		return RAZ;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate Calculate Crown Fraction Burned.To calculate CFB, we also
	//     need to calculate Critical surface intensity(CSI), and Surface fire
	//     rate of spread(RSO).The value of each of these equations can be
	//     returned to the calling function without unecessary additional
	//     calculations.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//   FMC : Foliar Moisture Content
	//   SFC : Surface Fuel Consumption
	//   CBH : Crown Base Height
	//   ROS : Rate of Spread
	//   option : Which variable to calculate(ROS, CFB, RSC, or RSI)
	// Returns :
	//   CFB, CSI, RSO depending on which option was selected.
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::CFBcalc(size_t FUELTYPE, double FMC, double SFC, double ROS, double CBH, string option)
	{
		assert(option == "CSI" || option == "RSO" || option == "CFB");


		//Eq. 56 (FCFDG 1992) Critical surface intensity
		double CSI = 0.001 * pow(CBH, 1.5) * pow(460 + 25.9 * FMC, 1.5);
		//Return at this point, if specified by caller
		if (option == "CSI")
			return CSI;

		//Eq. 57 (FCFDG 1992) Surface fire rate of spread(m / min)
		double RSO = CSI / (300 * SFC);
		//Return at this point, if specified by caller
		if (option == "RSO")
			return RSO;

		//Eq. 58 (FCFDG 1992) Crown fraction burned
		double CFB = 0;
		if (ROS > RSO)
			CFB = 1 - exp(-0.23 * (ROS - RSO));

		return CFB;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Total(Surface + Crown) Fuel Consumption by Fuel Type.
	//   All variables names are laid out in the same manner as FCFDG(1992) or
	//   Wotton et.al(2009)
	//
	//   Forestry Canada Fire Danger Group(FCFDG) (1992). "Development and 
	//   Structure of the Canadian Forest Fire Behavior Prediction System." 
	//   Technical Report ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//        CFL : Crown Fuel Load(kg / m ^ 2)
	//        CFB : Crown Fraction Burned(0 - 1)
	//        SFC : Surface Fuel Consumption(kg / m ^ 2)
	//         PC : Percent Conifer(%)
	//        PDF : Percent Dead Balsam Fir(%)
	//     option : Type of output(TFC, CFC, default = TFC)
	// Returns :
	//        TFC : Total(Surface + Crown) Fuel Consumption(kg / m ^ 2)
	//       OR
	//        CFC: Crown Fuel Consumption(kg / m ^ 2)
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::TFCcalc(size_t FUELTYPE, double CFL, double CFB, double SFC, double PC, double PDF, string option)
	{
		assert(option == "CFC" || option == "TFC");
		//Eq. 66a(Wotton 2009) - Crown Fuel Consumption(CFC)
		double CFC = CFL * CFB;
		//Eq. 66b(Wotton 2009) - CFC for M1/M2 types
		if (FUELTYPE == FUEL_M1 || FUELTYPE == FUEL_M2)
			CFC = PC / 100 * CFC;
		//Eq. 66c(Wotton 2009) - CFC for M3/M4 types
		if (FUELTYPE == FUEL_M3 || FUELTYPE == FUEL_M4)
			CFC = PDF / 100 * CFC;

		//Return CFC if requested
		if (option == "CFC")
			return CFC;

		//Eq. 67 (FCFDG 1992) - Total Fuel Consumption
		double TFC = SFC + CFC;
		return TFC;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate the Predicted Fire Intensity
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FC : Fuel Consumption(kg / m ^ 2)
	//   ROS : Rate of Spread(m / min)
	//   
	// Returns :
	//   FI : Fire Intensity(kW / m)
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::FIcalc(double FC, double ROS)
	{
		//Eq. 69 (FCFDG 1992) Fire Intensity(kW / m)
		double FI = 300 * FC * ROS;
		return FI;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Length to Breadth ratio of an elliptically shaped fire at
	//   elapsed time since ignition. Equations are from listed FCFDG(1992) and
	//   Wotton et.al. (2009), and are marked as such.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//         LB : Length to Breadth ratio
	//         HR : Time since ignition(hours)
	//        CFB : Crown Fraction Burned
	// Returns :
	//   LBt : Length to Breadth ratio at time since ignition
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::LBtcalc(size_t FUELTYPE, double LB, double HR, double CFB)
	{

		static const TFuel ALPHA_FUEL[] = { FUEL_C1, FUEL_O1A, FUEL_O1B, FUEL_S1, FUEL_S2, FUEL_S3, FUEL_D1 };
		bool bAlpha = (find(begin(ALPHA_FUEL), end(ALPHA_FUEL), FUELTYPE) != end(ALPHA_FUEL));

		//Eq. 72 (FCFDG 1992) - alpha constant value, dependent on fuel type
		double alpha = bAlpha ? 0.115 : 0.115 - 18.8 * pow(CFB, 2.5) * exp(-8 * CFB);

		//Eq. 81 (Wotton et.al. 2009) - LB at time since ignition
		double LBt = (LB - 1) * (1 - exp(-alpha * HR)) + 1;
		return LBt;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate the Flank Fire Spread Rate.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   ROS : Fire Rate of Spread(m / min)
	//   BROS : Back Fire Rate of Spread(m / min)
	//   LB : Length to breadth ratio
	//   
	// Returns :
	//   FROS : Flank Fire Spread Rate(m / min)
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::FROScalc(double ROS, double BROS, double LB)
	{
		//Eq. 89 (FCFDG 1992)
		double FROS = (ROS + BROS) / LB / 2;
		return(FROS);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Length to Breadth ratio of an elliptically shaped fire.
	//   Equations are from listed FCFDG(1992) except for errata 80 from
	//   Wotton et.al. (2009).
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//        WSV : The Wind Speed(km / h)
	// Returns :
	//   LB : Length to Breadth ratio
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::LBcalc(size_t FUELTYPE, double WSV)
	{
		//calculation is depending on if fuel type is grass(O1) or other fueltype
		double LB = 1.0 + 8.729 * pow(1 - exp(-0.030 * WSV), 2.155); //Eq. 79

		if (FUELTYPE == FUEL_O1A || FUELTYPE == FUEL_O1B)
		{
			//Correction to orginal Equation 80 is made here
			//Eq. 80a / 80b from Wotton 2009
			LB = (WSV >= 1.0) ? 1.1 * pow(WSV, 0.464) : 1.0; //Eq. 80 / 81
		}

		return(LB);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate the Back Fire Spread Rate.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//   FFMC : Fine Fuel Moisture Code
	//   BUI : Buildup Index
	//   WSV : Wind Speed Vector
	//   FMC : Foliar Moisture Content
	//   SFC : Surface Fuel Consumption
	//   PC : Percent Conifer
	//   PDF : Percent Dead Balsam Fir
	//   CC : Degree of Curing(just "C" in FCFDG 1992)
	//   CBH : Crown Base Height
	//
	// Returns :
	//   BROS : Back Fire Spread Rate
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::BROScalc(size_t FUELTYPE, double FFMC, double BUI, double WSV, double FMC, double SFC, double PC, double PDF, double CC, double CBH)
	{
		//Eq. 46 (FCFDG 1992)
		//Calculate the FFMC function from the ISI equation
		double m = 147.2 * (101 - FFMC) / (59.5 + FFMC);
		//Eq. 45 (FCFDG 1992)
		double fF = 91.9 * exp(-0.1386 * m) * (1.0 + pow(m, 5.31) / 4.93e7);
		//Eq. 75 (FCFDG 1992)
		//Calculate the Back fire wind function
		double BfW = exp(-0.05039 * WSV);
		//Calculate the ISI associated with the back fire spread rate
		//Eq. 76 (FCFDG 1992)
		double BISI = 0.208 * BfW * fF;
		//Eq. 77 (FCFDG 1992)
		//Calculate final Back fire spread rate
		double BROS = ROScalc(FUELTYPE, BISI, BUI, FMC, SFC, PC, PDF, CC, CBH);

		return BROS;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate Foliar Moisture Content on a specified day.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   LAT : Latitude(decimal degrees)
	//   LONG : Longitude(decimal degrees)
	//   ELV : Elevation(metres)
	//   DJ : Day of year(offeren referred to as julian date)
	//   D0 : Date of minimum foliar moisture content
	//   
	// Returns :
	//   FMC : Foliar Moisture Content
	//
	// if D0, date of min FMC, is not known then D0 = 0.
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::FMCcalc(double LAT, double LONG, double ELV, size_t DJ, size_t D0)
	{
		if (D0 <= 0)
		{
			//Calculate Normalized Latitude
			//Eqs. 1 & 3 (FCFDG 1992)

			double LATN = (ELV <= 0) ?
				46 + 23.4 * exp(-0.0360 * (150 - LONG)) :
				43 + 33.7 * exp(-0.0351 * (150 - LONG));

			//Calculate Date of minimum foliar moisture content
			//Eqs. 2 & 4 (FCFDG 1992)
			//Round D0 to the nearest integer because it is a date
			D0 = (ELV <= 0) ?
				Round(151 * (LAT / LATN), 0) :
				Round(142.1 * (LAT / LATN) + 0.0172 * ELV, 0);
		}


		//Number of days between day of year and date of min FMC
		//Eq. 5 (FCFDG 1992)
		int ND = abs(int(DJ) - int(D0));

		//Calculate final FMC
		//Eqs. 6, 7, &8 (FCFDG 1992)
		double FMC = (ND < 30) ? 85 + 0.0189 * pow(ND, 2) :
			(ND >= 30 && ND < 50) ? 32.9 + 3.17 * ND - 0.0288 * pow(ND, 2) : 120;

		return FMC;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate c6(Conifer plantation) Fire Spread.C6 is a special case, and
	//     thus has it's own function. To calculate C6 fire spread, this function 
	//     also calculates and can return ROS, CFB, RSC, or RSI by specifying in
	//     the option parameter.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//   ISI : Initial Spread Index
	//   BUI : Buildup Index
	//   FMC : Foliar Moisture Content
	//   SFC : Surface Fuel Consumption
	//   CBH : Crown Base Height
	//   ROS : Rate of Spread
	//   CFB : Crown Fraction Burned
	//   RSC : Crown Fire Spread Rate(m / min)
	//   option : Which variable to calculate(ROS, CFB, RSC, or RSI)
	//
	// Returns :
	//   ROS, CFB, RSC or RSI depending on which option was selected
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::C6calc(size_t FUELTYPE, double ISI, double BUI, double FMC, double SFC, double CBH, /*double ROS, double CFB, double RSC, */string option)
	{
		assert(option == "RSI" || option == "RSC" || option == "ROS" || option == "CFB");

		//Average foliar moisture effect
		double FMEavg = 0.778;
		//Eq. 59 (FCFDG 1992) Crown flame temperature(degrees K)
		double tt = 1500 - 2.75 * FMC;
		//Eq. 60 (FCFDG 1992) Head of ignition(kJ / kg)
		double H = 460 + 25.9 * FMC;
		//Eq. 61 (FCFDG 1992) Average foliar moisture effect
		double FME = pow((1.5 - 0.00275 * FMC), 4.) / (460 + 25.9 * FMC) * 1000;
		//Eq. 62 (FCFDG 1992) Intermediate surface fire spread rate
		double RSI = 30 * pow(1 - exp(-0.08 * ISI), 3.0);
		//Return at this point, if specified by caller
		if (option == "RSI")
			return RSI;

		//Eq. 63 (FCFDG 1992) Surface fire spread rate(m / min)
		double RSS = RSI * BEcalc(FUELTYPE, BUI);
		//Eq. 64 (FCFDG 1992) Crown fire spread rate(m / min)
		double RSC = 60 * (1 - exp(-0.0497 * ISI)) * FME / FMEavg;
		//Return at this point, if specified by caller
		if (option == "RSC")
			return RSC;

		//Crown Fraction Burned
		double CFB = 0;
		if (RSC > RSS)
			CFB = CFBcalc(FUELTYPE, FMC, SFC, RSS, CBH);

		//Return at this point, if specified by caller
		if (option == "CFB")
			return CFB;

		//Eq. 65 (FCFDG 1992) Calculate Rate of spread(m / min)
		double ROS = (RSC > RSS) ? RSS + (CFB)*(RSC - RSS) : RSS;

		return ROS;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Rate of Spread prediction based on fuel type and FWI
	//   conditions.Equations are from listed FCFDG(1992) and Wotton et.al.
	//   (2009), and are marked as such.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	//   Wotton, B.M., Alexander, M.E., Taylor, S.W. 2009. Updates and revisions to
	//   the 1992 Canadian forest fire behavior prediction system.Nat.Resour.
	//   Can., Can.For.Serv., Great Lakes For.Cent., Sault Ste.Marie, Ontario,
	//   Canada.Information Report GLC - X - 10, 45p.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//        ISI : Intiial Spread Index
	//        BUI : Buildup Index
	//        FMC : Foliar Moisture Content
	//        SFC : Surface Fuel Consumption(kg / m ^ 2)
	//         PC : Percent Conifer(%)
	//        PDF : Percent Dead Balsam Fir(%)
	//         CC : Constant
	//        CBH : Crown to base height(m)
	// Returns :
	//   ROS : Rate of spread(m / min)
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::ROScalc(size_t FUELTYPE, double ISI, double BUI, double FMC, double SFC, double PC, double PDF, double CC, double CBH)
	{
		//Set up some data vectors

		static const TFuel RSI_FUEL[] = { FUEL_C1, FUEL_C2, FUEL_C3, FUEL_C4, FUEL_C5, FUEL_C7, FUEL_D1, FUEL_S1, FUEL_S2, FUEL_S3 };
		double RSI = -1;

		if (find(begin(RSI_FUEL), end(RSI_FUEL), FUELTYPE) != end(RSI_FUEL))
		{
			//Calculate RSI(set up data vectors first)
			//Eq. 26 (FCFDG 1992) - Initial Rate of Spread for Conifer and Slash types

			RSI = a[FUELTYPE] * pow(1 - exp(-b[FUELTYPE] * ISI), c0[FUELTYPE]);
		}
		else if (FUELTYPE == FUEL_M1)
		{
			//Eq. 27 (FCFDG 1992) - Initial Rate of Spread for M1 Mixedwood type
			RSI = PC / 100 * ROScalc(FUEL_C2, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)
				+ (100 - PC) / 100 * ROScalc(FUEL_D1, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH);
		}
		else if (FUELTYPE == FUEL_M2)
		{
			//Eq. 27 (FCFDG 1992) - Initial Rate of Spread for M2 Mixedwood type
			RSI = PC / 100 * ROScalc(FUEL_C2, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)
				+ 0.2*(100 - PC) / 100 * ROScalc(FUEL_D1, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH);
		}
		else if (FUELTYPE == FUEL_M3)
		{
			//double RSI_m3 = -99;
			//Initial Rate of Spread for M3 Mixedwood
			//Eq. 30 (Wotton et.al 2009)
			double RSI_m3 = a[FUEL_M3] * pow(1 - exp(-b[FUEL_M3] * ISI), c0[FUEL_M3]);

			//Eq. 29 (Wotton et.al 2009)
			RSI = PDF / 100 * RSI_m3 + (1 - PDF / 100) *
				ROScalc(FUEL_D1, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH);
		}
		//Initial Rate of Spread for M4 Mixedwood
		//double RSI_m4 = -99;
		else if (FUELTYPE == FUEL_M4)
		{
			//Eq. 30 (Wotton et.al 2009)
			double RSI_m4 = a[FUEL_M4] * pow(1 - exp(-b[FUEL_M4] * ISI), c0[FUEL_M4]);

			//Eq. 33 (Wotton et.al 2009)
			RSI = PDF / 100 * RSI_m4 + 0.2 * (1 - PDF / 100)*
				ROScalc(FUEL_D1, ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH);
		}
		else if (FUELTYPE == FUEL_O1A || FUELTYPE == FUEL_O1B)
		{

			//Eq. 35b(Wotton et.al. 2009) - Calculate Curing function for grass
			double CF = (CC < 58.8) ? 0.005 * (exp(0.061 * CC) - 1) : 0.176 + 0.02 * (CC - 58.8);

			//Eq. 36 (FCFDG 1992) - Calculate Initial Rate of Spread for Grass
			RSI = a[FUELTYPE] * pow(1 - exp(-b[FUELTYPE] * ISI), c0[FUELTYPE]) * CF;
		}

		//Calculate C6 separately
		double ROS = (FUELTYPE == FUEL_C6) ? C6calc(FUELTYPE, ISI, BUI, FMC, SFC, CBH, "ROS") : BEcalc(FUELTYPE, BUI) * RSI;

		//add a constraint
		ROS = max(0.000001, ROS);

		return ROS;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Buildup Effect on Fire Spread Rate.
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//   BUI : The Buildup Index value
	// Returns :
	//   BE : The Buildup Effect
	//

	double CFBP::BEcalc(size_t FUELTYPE, double BUI)
	{
		//Fuel Type String represenations
		//The average BUI for the fuel type - as referenced by the "d" list above
		static const double BUIo[NB_FUEL_TYPE] = { 72, 64, 62, 66, 56, 62, 106, 32, 50, 50, 50, 50, 38, 63, 31, 01, 01 };
		//Proportion of maximum possible spread rate that is reached at a standard BUI
		static const double Q[NB_FUEL_TYPE] = { 0.9, 0.7, 0.75, 0.8, 0.8, 0.8, 0.85, 0.9, 0.8, 0.8, 0.8, 0.8, 0.75, 0.75, 0.75, 1.0, 1.0 };

		//Eq. 54 (FCFDG 1992) The Buildup Effect
		double BE = 1;
		if (BUI > 0 && BUIo[FUELTYPE] > 0)
			BE = exp(50 * log(Q[FUELTYPE]) * (1 / BUI - 1 / BUIo[FUELTYPE]));

		return BE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Computes the Rate of Spread prediction based on fuel type and FWI
	//   conditions at elapsed time since ignition.Equations are from listed
	//   FCFDG(1992).
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//      ROSeq : Equilibrium Rate of Spread(m / min)
	//         HR : Time since ignition(hours)
	//        CFB : Crown Fraction Burned
	// Returns :
	//   ROSt : Rate of Spread at time since ignition
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::ROStcalc(size_t FUELTYPE, double ROSeq, double HR, double CFB)
	{
		//Eq. 72 - alpha constant value, dependent on fuel type
		static const TFuel ALPHA_FUEL[] = { FUEL_C1, FUEL_O1A, FUEL_O1B, FUEL_S1, FUEL_S2, FUEL_S3, FUEL_D1 };
		bool bAlpha = (find(begin(ALPHA_FUEL), end(ALPHA_FUEL), FUELTYPE) != end(ALPHA_FUEL));
		double alpha = (bAlpha) ? 0.115 : 0.115 - 18.8 * pow(CFB, 2.5) * exp(-8 * CFB);

		//Eq. 70 - Rate of Spread at time since ignition
		double ROSt = ROSeq * (1 - exp(-alpha * HR));
		return ROSt;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Description:
	//   Calculate the Head fire spread distance at time t.In the documentation
	//   this variable is just "D".
	//
	//   All variables names are laid out in the same manner as Forestry Canada
	//   Fire Danger Group(FCFDG) (1992).Development and Structure of the
	//   Canadian Forest Fire Behavior Prediction System." Technical Report 
	//   ST - X - 3, Forestry Canada, Ottawa, Ontario.
	//
	// Args:
	//   FUELTYPE : The Fire Behaviour Prediction FuelType
	//   ROSeq : The predicted equilibrium rate of spread(m / min)
	//   HR(t) : The elapsed time(min)
	//   CFB : Crown Fraction Burned
	//   
	// Returns :
	//   DISTt : Head fire spread distance at time t
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	double CFBP::DISTtcalc(size_t FUELTYPE, double ROSeq, double HR, double CFB)
	{
		//Eq. 72 (FCFDG 1992)
		//Calculate the alpha constant for the DISTt calculation
		static const TFuel ALPHA_FUEL[] = { FUEL_C1, FUEL_O1A, FUEL_O1B, FUEL_S1, FUEL_S2, FUEL_S3, FUEL_D1 };
		bool bAlpha = (find(begin(ALPHA_FUEL), end(ALPHA_FUEL), FUELTYPE) != end(ALPHA_FUEL));
		double alpha = (bAlpha) ? 0.115 : 0.115 - 18.8 * pow(CFB, 2.5) * exp(-8 * CFB);

		//Eq. 71 (FCFDG 1992) Calculate Head fire spread distance
		double DISTt = ROSeq * (HR + exp(-alpha * HR) / alpha - 1 / alpha);

		return DISTt;
	}

}//WBSF

