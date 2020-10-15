#pragma once

#include <string>

namespace WBSF
{
	//C1 Spruce-Lichen Woodland
	//C2 Boreal Spruce
	//C3 Mature Jack or Lodgepole Pine
	//C4 Immature Jack or Lodgepole Pine
	//C5 Red and White Pine
	//C6 Conifer Plantation
	//C7 Ponderosa Pine / Douglas Fir
	//D1 Leafless Aspen
	//M1 Boreal Mixedwood - Leafless 
	//M2 Boreal Mixedwood - Green 
	//M3 Dead Balsam Fir Mixedwood - Leafless 
	//M4 Dead Balsam Fir Mixedwood - Green 
	//S1 Jack or Lodgepole Pine Slash
	//S2 White Spruce / Balsam Slash 
	//S3 Coastal Cedar / Hemlock / Douglas-Fir Slash
	//O1A Matted Grass 
	//O1B Standing Grass
	enum TAcceleration { AC_LINE, AC_POINT };
	enum TFuel { FUEL_UNKNOWNS = -1, FUEL_C1, FUEL_C2, FUEL_C3, FUEL_C4, FUEL_C5, FUEL_C6, FUEL_C7, FUEL_D1, FUEL_M1, FUEL_M2, FUEL_M3, FUEL_M4, FUEL_S1, FUEL_S2, FUEL_S3, FUEL_O1A, FUEL_O1B, NB_FUEL_TYPE };

	class CFBPInput
	{
	public:

		double LAT = 55;   //Latitude [decimal degrees]
		double LONG = -120;//Longitude [decimal degrees]
		double ELV = 0;//Elevation [meters above sea level]
		double GS = 0;//GS	Ground Slope [percent]
		double ASPECT = 0;//Aspect	Aspect of the slope [decimal degrees]

		size_t fuel_type = FUEL_C2;
		TAcceleration accel = AC_LINE;//Acceleration: 1 = point, 0 = line
		size_t DJ = 180;//Julian day
		size_t D0 = 0;//Julian day of minimum Foliar Moisture Content

		bool BUIeff = 1;//Buildup Index effect: 1=yes, 0=no
		double hr = 1;//Hours since ignition

		double FFMC = 90;//Fine fuel moisture code [FWI System component]
		double ISI = 0;//Initial spread index [FWI System component]
		double BUI = 60;//Buildup index [FWI System component]

		double WS = 10;//Wind speed [km/h]
		double WD = 0;//Wind direction [decimal degrees]
		double theta = 0;//Elliptical direction of calculation [degrees]

		double PC = 50;     //Percent Conifer for M1/M2 [percent]
		double PDF = 35;	//Percent Dead Fir for M3/M4 [percent]
		double cc = 80;		//Percent Cured for O1a/O1b [percent]
		double GFL = 0.35;	//Grass Fuel Load [kg/m^2]
		double CBH = 3;//Crown to Base Height [m]
		double CFL = 1;//Crown Fuel Load [kg/m^2]
		double FMC = 0;//Foliar Moisture Content if known [percent]
		double SD = 0;//Fuel Type Stand Density [stems/ha]
		double SH = 0;//Fuel Type Stand Height [m]

	};

	class CFBPOutput
	{
	public:

		//Primary FBP output includes the following 8 variables:
		double CFB = 0;//Crown Fraction Burned by the head fire
		double CFC = 0;//Crown Fuel Consumption [kg/m^2]
		char FD = 0;//Fire description (S=Surface, I=Intermittent, C=Crown)
		double HFI = 0;//Head Fire Intensity [kW/m]
		double RAZ = 0;//Spread direction azimuth [degrees]
		double ROS = 0;//Equilibrium Head Fire Rate of Spread [m/min]
		double SFC = 0;//Surface Fuel Consumption [kg/m^2]
		double TFC = 0;//Total Fuel Consumption [kg/m^2]

		//Secondary FBP System outputs include the following 34 raster layers. In order to calculate the reliable secondary outputs, depending on the outputs, optional inputs may have to be provided.
		double BE = 0;//BUI effect on spread rate
		double SF = 0;//Slope Factor (multiplier for ROS increase upslope)
		double ISI = 0;//Initial Spread Index[FWI System component]
		double FFMC = 0;//Fine fuel moisture code [FWI System component]
		double FMC = 0;//Foliar Moisture Content [%]
		double D0 = 0;//Julian Date of minimum FMC
		double RSO = 0;//Critical spread rate for crowning [m/min]
		double CSI = 0;//Critical Surface Intensity for crowning [kW/m]
		double FROS = 0;//Equilibrium Flank Fire Rate of Spread [m/min]
		double BROS = 0;//Equilibrium Back Fire Rate of Spread [m/min]
		double HROSt = 0;//Head Fire Rate of Spread at time hr [m/min]
		double FROSt = 0;//Flank Fire Rate of Spread at time hr [m/min]
		double BROSt = 0;//Back Fire Rate of Spread at time hr [m/min]
		double FCFB = 0;//Flank Fire Crown Fraction Burned
		double BCFB = 0;//Back Fire Crown Fraction Burned
		double FFI = 0;//Equilibrium Spread Flank Fire Intensity [kW/m]
		double BFI = 0;//Equilibrium Spread Back Fire Intensity [kW/m]
		double FTFC = 0;//Flank Fire Total Fuel Consumption [kg/m^2]
		double BTFC = 0;//Back Fire Total Fuel Consumption [kg/m^2]
		double TI = 0;//Time to Crown Fire Initiation [hrs since ignition]
		double FTI = 0;//Time to Flank Fire Crown initiation [hrs since ignition]
		double BTI = 0;//Time to Back Fire Crown initiation [hrs since ignition]
		double LB = 0;//Length to Breadth ratio
		double LBt = 0;//Length to Breadth ratio after elapsed time hr
		double WSV = 0;//Net vectored wind speed [km/hr]
		double DH = 0;//Head Fire Spread Distance after time hr [m]
		double DB = 0;//Back Fire Spread Distance after time hr [m]
		double DF = 0;//Flank Fire Spread Distance after time hr [m]
		double TROS = 0;//Equilibrium Rate of Spread at bearing theta [m/min]
		double TROSt = 0;//Rate of Spread at bearing theta at time t [m/min]
		double TCFB = 0;//Crown Fraction Burned at bearing theta
		double TFI = 0;//Fire Intensity at bearing theta [kW/m]
		double TTFC = 0;//Total Fuel Consumption at bearing theta [kg/m^2]
		double TTI = 0;//Time to Crown Fire initiation at bearing theta [hrs since ignition]
	};


	class CFBP
	{
	public:
		static const char* FUEL_NAMES[NB_FUEL_TYPE];
		static const double CBHs[NB_FUEL_TYPE];
		static const double CFLs[NB_FUEL_TYPE];
		static const double a[NB_FUEL_TYPE];
		static const double b[NB_FUEL_TYPE];
		static const double c0[NB_FUEL_TYPE];
		static const double PDF100;//100 % Dead Balsam Fir
		static const double NoBUI;

		static TFuel GetFuelType(const std::string& fuel_type);

		static void Execute(CFBPInput input, CFBPOutput& output);

		static double FMCcalc(double LAT, double LONG, double ELV, size_t DJ, size_t D0);
		static double SFCcalc(size_t FUELTYPE, double FFMC, double BUI, double PC, double GFL);
		static double Slopecalc(size_t FUELTYPE, double FFMC, double BUI, double WS, double WAZ, double GS, double SAZ, double FMC, double SFC, double PC, double PDF, double CC, double CBH, double ISI, std::string output = "RAZ");
		static double TFCcalc(size_t FUELTYPE, double CFL, double CFB, double SFC, double PC, double PDF, std::string option = "TFC");
		static double CFBcalc(size_t FUELTYPE, double FMC, double SFC, double ROS, double CBH, std::string option = "CFB");
		static double FIcalc(double FC, double ROS);
		static double LBtcalc(size_t FUELTYPE, double LB, double HR, double CFB);
		static double ROScalc(size_t FUELTYPE, double ISI, double BUI, double FMC, double SFC, double PC, double PDF, double CC, double CBH);
		static double BROScalc(size_t FUELTYPE, double FFMC, double BUI, double WSV, double FMC, double SFC, double PC, double PDF, double CC, double CBH);
		static double FROScalc(double ROS, double BROS, double LB);
		static double LBcalc(size_t FUELTYPE, double WSV);
		static double C6calc(size_t FUELTYPE, double ISI, double BUI, double FMC, double SFC, double CBH, /*double ROS, double CFB, double RSC, */std::string option);
		static double BEcalc(size_t FUELTYPE, double BUI);
		static double ROStcalc(size_t FUELTYPE, double ROSeq, double HR, double CFB);
		static double DISTtcalc(size_t FUELTYPE, double ROSeq, double HR, double CFB);

	};


}