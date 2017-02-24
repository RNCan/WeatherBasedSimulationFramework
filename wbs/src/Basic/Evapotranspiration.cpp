//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//  Class:	CThornthwaiteET
//			CBlaneyCriddleET
//			CTurcET
//			CPriestleyTaylorET
//			CModifiedPriestleyTaylorET
//			CASCE_ETsz
//			Hamon
//			CPenmanMonteithET
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 02-10-2013	Rémi Saint-Amant  Update Priestley-Taylor ET with new hourly weather structure
// 15-11-2010   Rémi Saint-Amant  Add Hamon Evapotranspiration
// 27-05-2008	Rémi Saint-Amant  Add of Priestley-Taylor ET
// 04-03-2004	Rémi Saint-Amant  Removed the old GetVaporPressureDeficit
// 20-11-2003	Rémi Saint-Amant  Initial Version
//******************************************************************************
#include "stdafx.h"
#include "Evapotranspiration.h"
#include "WeatherStation.h"
#include "Basic/UtilMath.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;


namespace WBSF
{

	static const double π = PI;

extern const char ET_HEADER[] = "ETo";
//extern const char EXTENDED_ET_HEADER[] = "ETo";

CETFactory::CETFactory()
{
}


bool CETFactory::Register(const string &name, CreateETFn pfnCreate)
{
	bool bRep = true;
	Get().m_CS.Enter();
	
	FactoryMap::iterator it = Get().m_factoryMap.find(name);
	bRep = it == Get().m_factoryMap.end();
	if (bRep)
		Get().m_factoryMap[name] = pfnCreate;
	
	Get().m_CS.Leave();

	return bRep;
}

CETPtr CETFactory::CreateET(const string &name)
{

	Get().m_CS.Enter();

	CETPtr pModel;
	FactoryMap::iterator it = Get().m_factoryMap.find(name);
	if (it != Get().m_factoryMap.end())
		pModel.reset(it->second());

	Get().m_CS.Leave();

	return pModel;
}

bool CETFactory::IsRegistered(const std::string &name)
{
	return Get().m_factoryMap.find(name) != Get().m_factoryMap.end();
}


//*************************************************************


//*************************************************************
CETOptions::CETOptions()
{
}

//*************************************************************
//CThornthwaiteET

const bool CThornthwaiteET::AUTO_REGISTER = CETFactory::Register("Thornthwaite", &CThornthwaiteET::Create);
CThornthwaiteET::CThornthwaiteET(size_t type)
{
	m_type=POTENTIEL_STANDARD;
}

double CThornthwaiteET::GetCorrectionFactor(double lat, size_t m)
{
	//http://en.wikipedia.org/wiki/Potential_evaporation
	//[(L/12)*(N/30)]
	//N is the number of days in the month being calculated
	//L is the average day length (hours) of the month being calculated

	CStatistic stat;
	for (size_t d = 0; d<GetNbDayPerMonth(m); d++)
		stat+=GetDayLength(lat,m,d)/3600;

	double L = stat[MEAN];
	double N = stat[NB_VALUE];
	return (L/12)*(N/30);
}


ERMsg CThornthwaiteET::SetOptions(const CETOptions& options)
{
	ERMsg msg;

	if (options.OptionExist("ThornthwaiteType"))
		m_type = ToShort(options.GetOption("ThornthwaiteType"));
	
	return msg;
}

double CThornthwaiteET::GetI(const CYear& weather)
{
	double I = 0;
	for(size_t  m=0; m<12; m++)
	{
		double mean = weather[m][H_TNTX][MEAN];
		if(mean>0) 
			I += pow(mean/5.,1.514);
	}
	
	return I;
}

double CThornthwaiteET::GetAlpha(double I)
{
	//compute alpha, see wikipedia: http://en.wikipedia.org/wiki/Potential_evaporation
	double alpha = 0.49239+0.01792*I-0.0000771*I*I+0.000000675*I*I*I;

	return alpha;
}

double CThornthwaiteET::GetET(const CMonth& weather, double I)
{
	double ET=0;
	double alpha = GetAlpha(I);

	//compute Et for each month and A
	double mean = weather[H_TNTX][MEAN];
	if(mean>0) 
	{
		ET = 16*pow(10.*mean/I, alpha);
		if (m_type == POTENTIEL_ADJUSTED)
		{
			//The resulting potential evapotranspiration value, determined using the graph or formula, is for 360 hours 
			//of sunlight per month, and must be adjusted for the number of days per month and length of day (a function of latitude).
			//The standard potential evapotranspiration value should be multiplied by the appropriate
			//correction factor given in Table 13 to produce the potential evapotranspiration value for the station.
			size_t m = weather.GetTRef().GetMonth();
			ET *= GetCorrectionFactor(weather.GetLocation().m_lat, m);

			//m_evapotranspiration[m] *= [(L/12)*(N/30)]
			//N is the number of days in the month being calculated
			//L is the average day length (hours) of the month being calculated
		}
	}//if mean>0

	return ET;
}

void CThornthwaiteET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);
	for (size_t y = 0; y<weather.size(); y++)
	{
		double I = GetI(weather[y]);

		//compute Et for each month and A
		for (size_t m = 0; m<12; m++)
		{
			double monthlyET = GetET(weather[y][m], I);

			//ET distributed over time
			CTPeriod p = weather[y][m].GetEntireTPeriod();
			size_t nbTRef = p.size();
			for (CTRef TRef = p.Begin(); TRef<=p.End(); TRef++)
				output[TRef][S_ET] = monthlyET / nbTRef;
		}//m
	}//y
}

//return Water Deficit en mm of water
/*
double CThornthwaiteET::GetWaterDeficit(const CWeatherYear& weather)const
{
	_ASSERTE( weather.GetLocation().m_latitude!=-999);
	_ASSERTE( weather[0][0].GetPpt() >= 0);
	

	double ET[12]={0};
	ComputeETOld( weather, weather.GetLocation().m_latitude, m_type, ET);


	double A = 0;
	//calculer Et pour le mois et A
	for(int m=0; m<12; m++) 
    {
		if(weather[m].GetStat(STAT_T_MN, MEAN)>0.) 
        {
			//precipitation in mm
			double A_tmp=(ET[m]-weather[m].GetStat(STAT_PRCP, SUM));
			if(A_tmp>0.) 
                A += A_tmp; 
		}
	}

	return A;
}
void CThornthwaiteET::GetWaterDeficit(const CWeatherYear& weather, double WD[12])const
{
	_ASSERTE( weather.GetLocation().m_latitude!=-999);
	_ASSERTE( weather[0][0].GetPpt() >= 0);
	

	double ET[12]={0};
	ComputeETOld( weather, weather.GetLocation().m_latitude, m_type, ET);


	double A = 0;
	//calculer Et pour le mois et A
	for(int m=0; m<12; m++) 
    {
		WD[m]=0;
		
		if(weather[m].GetStat(STAT_T_MN, MEAN)>0.) 
        {
			//precipitation in mm
			//double A_tmp=;
			WD[m] = max(0.0, ET[m]-weather[m].GetStat(STAT_PRCP, SUM));
			//if(A_tmp>0.) 
              //  A += A_tmp; 
		}
	}

	//return A;
}
*/

//*************************************************************
//CBlaneyCriddleET



//Correction factors for monthly sunshine duration for multiplication
//of the standard potential evapotranspiration
//  Lat, Jan., Feb., Mar., Apr., May., Jun., Jul., Aug., Sep., Oct., Nov., Dec.
const CBlaneyCriddleET::CCorrectionFactorsMonth CBlaneyCriddleET::CorrectionFactors[10] = 
{
	{60, .047, .057, .081, .096, .117, .124, .123, .107, .086, .070, .050, .042},
	{50, .060, .063, .082, .092, .107, .109, .110, .100, .085, .075, .061, .056},
	{40, .067, .066, .082, .089, .099, .100, .101, .094, .083, .077, .067, .075},
	{20, .073, .070, .084, .087, .095, .095, .097, .092, .083, .080, .072, .072},
	{10, .081, .075, .085, .084, .088, .086, .089, .087, .082, .083, .079, .081},
	{0 , .085, .077, .085, .082, .085, .082, .085, .085, .082, .085, .082, .085},
   {-10, .089, .079, .085, .081, .082, .079, .081, .083, .082, .086, .085, .088},
   {-20, .092, .081, .086, .079, .079, .074, .078, .080, .081, .088, .089, .093},
   {-30, .097, .083, .086, .077, .074, .070, .073, .078, .081, .090, .092, .099},
   {-40, .102, .086, .087, .075, .070, .064, .068, .074, .080, .092, .097, .105}
};

//Crop Growing Season (from U.S. soil conservation Service 1970)
//Jan. Feb. Mar. Apr. May June July Aug. Sept. Oct. Nov. dec.
//Beginning Mean Temp. (°C)
//End Mean Temp. (°C)
//Max. Length (days)
//PERENNIALS
const CBlaneyCriddleET::CCropFactorsMonth CBlaneyCriddleET::CropFactors[] = 
{
	{"Pasture Grass",	  7, 7, -999, 0.49, 0.57, 0.73, 0.85, 0.90, 0.92, 0.92, 0.91, 0.87, 0.79, 0.67, 0.55},
	{"Alfalfa",			 10,-2, -999, 0.63, 0.73, 0.86, 0.99, 1.08, 1.13, 1.11, 1.06, 0.99, 0.91, 0.78, 0.64},
	{"Grapes",			 13,10, -999, 0.20, 0.24, 0.33, 0.50, 0.71, 0.80, 0.80, 0.76, 0.61, 0.50, 0.35, 0.23},
	{"Deciduous Orchard",10, 7, -999, 0.17, 0.25, 0.40, 0.63, 0.88, 0.96, 0.95, 0.82, 0.54, 0.30, 0.19, 0.15}
};

//PERCENT of GROWING SEASON
//ANNUALS 0-10 10-20 20-30 30-40 40-50 50-60 60-70 70-80 80-90 90-100
//Small vegetables 16 0 100 0.33 0.47 0.64 0.74 0.80 0.82 0.82 0.76 0.66 0.48
//Peas 100 0.54 0.65 0.80 0.97 1.08 1.12 1.11 1.08 1.04 0.98
//Potatoes 16 0 (frost) 130 0.36 0.45 0.59 0.85 1.09 1.26 1.35 1.37 1.34 1.27
//Sugar beets -2 (frost) -2 (frost) 180 0.46 0.54 0.69 0.87 1.03 1.16 1.24 1.24 1.18 1.10
//Grain corn (maize) 13 0 (frost) 140 0.46 0.54 0.64 0.82 1.00 1.08 1.08 1.03 0.97 0.89
//Silage corn(maize) 13 0 (frost) 140 0.45 0.50 0.59 0.71 0.90 1.03 1.07 1.07 1.04 1.00
//Sweet corn(maize) 13 0 (frost) 140 0.43 0.52 0.62 0.81 1.00 1.07 1.08 1.07 1.04 1.02
//Spring grain 7 0 (frost) 130 0.36 0.58 0.82 1.04 1.25 1.31 1.18 0.87 0.49 0.13
//Grain sorghum 16 0 130 0.32 0.47 0.72 0.93 1.07 1.04 0.94 0.82 0.70 0.60
//Soybeans 140 0.22 0.30 0.37 0.48 0.63 0.84 0.98 1.02 0.83 0.72
//Cotton 17 0 (frost) 240 0.22 0.28 0.40 0.64 0.90 1.01 1.00 0.88 0.73 0.57


double CBlaneyCriddleET::GetCorrectionFactor(double lat, size_t month)
{
    size_t index = 9;
	for (size_t i = 0; i<10; i++)
    {
        if( lat+5 > CorrectionFactors[i].m_latitude )
        {
            index = i;
            break;
        }
    }

    _ASSERTE( index >= 0 && index<9);
    _ASSERTE( month >= 0 && month<12);
    return CorrectionFactors[index ].m_cf[month];
}

const bool CBlaneyCriddleET::AUTO_REGISTER = CETFactory::Register("BlaneyCriddle", &CBlaneyCriddleET::Create);
CBlaneyCriddleET::CBlaneyCriddleET()
{
	m_cropType = PASTURE_GRASS;
}

ERMsg CBlaneyCriddleET::SetOptions(const CETOptions& options)
{
	ERMsg msg;

	if (options.OptionExist("BlaneyCriddleCropType"))
		m_cropType = (TCrop)ToShort(options.GetOption("BlaneyCriddleCropType"));

	return msg;
}

void CBlaneyCriddleET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);
	for (size_t y = 0; y<weather.size(); y++)
	{
		//compute Et for each month and A
		for (size_t m = 0; m<12; m++)
		{
			double Tmean = weather[y][m][H_TNTX][MEAN];

			//get evapotranspiration in mm
			double F1 = GetCropFactor(m_cropType, m);
			double F2 = GetCorrectionFactor(weather.m_lat, m);
			double monthlyET = max(0.0, 10 * (0.142*Tmean + 1.095)*(Tmean + 17.8)*F1*F2);
				
			//ET distributed over time
			CTPeriod p = weather[y][m].GetEntireTPeriod();
			size_t nbTRef = p.size();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				output[TRef][S_ET] = monthlyET / nbTRef;
		}//m
	}//y

}

//*************************************************************
//CTurcET


const bool CTurcET::AUTO_REGISTER = CETFactory::Register("Turc", &CTurcET::Create);
CTurcET::CTurcET()
{
}

//
//Evapotranspiration after Turc [mm day-1]:
//T		:	Average air temperature for the given time interval [°C]
//Rg	:	Global radiation [MJ m-2 d-1]
//RH	:	Average air humidity[%]
//Parameter
//a = 0.31 [m2 MJ-1 mm-1]
//b = 2.094 [MJ m-2 d-1]
void CTurcET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	static const double a = 0.31;	//[m2 MJ-1 mm-1]
	static const double b = 2.094;	//[MJ m-2 d-1]

	CTPeriod p = weather.GetEntireTPeriod();
	
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);
	for (size_t y = 0; y<weather.size(); y++)
	{
		//compute Et for each month 
		for (size_t m = 0; m<weather[y].size(); m++)
		{
			for (size_t d = 0; d < weather[y][m].size(); d++)
			{
				double T = weather[y][m][d][H_TNTX][MEAN];
				double Ea = weather[y][m][d][H_EA2][MEAN];	//vapor pressure [Pa]
				double Es = weather[y][m][d][H_ES2][MEAN];	//vapor pressure [Pa]
				double RH = max(1.0,min(100.0, Ea / Es * 100.0));//weather[y][m][d][H_RELH][MEAN];
				double Rg = weather[y][m][d][H_SRMJ][SUM]; //solar radiation in MJ/(m²·d)
				double C = RH>=50?1:1+(50-RH)/70;

				double dailyET = 0;
				if (T > 0)
					dailyET = a*C*(Rg + b)*T / (T + 15);

				//ET distributed over time
				CTPeriod p = weather[y][m][d].GetEntireTPeriod();
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					output[TRef][S_ET] = dailyET / p.size();
			}//d
		}//m
	}//y
}

//*************************************************************
//CPriestleyTaylorET


//From J.B Fisher et al. 2005 and Komatsu, 2005
const CPriestleyTaylorET::CAlpha CPriestleyTaylorET::PRE_DEFINE_ALHA[NB_ALPHA] =
{
	{ 1.57, "Strongly advective conditions", "Jury and Tanner, 1975" },
	{ 1.29, "Grass (soil at field capacity)", "Mukammal and Neumann, 1977" },
	{ 1.27, "Irrigated ryegrass", "Davies and Allen, 1973" },
	{ 1.26, "Saturated surface", "Priestley and Taylor, 1972" },
	{ 1.26, "Open-water surface", "Priestley and Taylor, 1972" },
	{ 1.26, "Wet meadow", " Stewart and Rouse, 1977" },
	{ 1.18, "Wet Douglas-fir forest", "McNaughton and Black, 1973" },
	{ 1.12, "Short grass", " De Bruin and Holtslag, 1982" },
	{ 1.09, "Boreal broad-leaved", "Komatsu, 2005" },
	{ 1.05, "Douglas-fir forest", "McNaughton and Black, 1973" },
	{ 1.04, "Bare soil surface", "Barton, 1979" },
	{ 0.90, "Mixed reforestation (water limited)", "Flint and Childs, 1991" },
	{ 0.87, "Ponderosa pine (water limited, daytime)", "Fisher, 2005" },
	{ 0.86, "Tropical broad-leaved", "Komatsu, 2005" },
	{ 0.84, "Douglas-fir forest (unthinned)", "Black, 1979" },
	{ 0.83, "Broad-leaved", "Komatsu, 2005" },
	{ 0.82, "Temperate broad-leaved", "Komatsu, 2005" },
	{ 0.80, "Douglas-fir forest (thinned)", "Black, 1979" },
	{ 0.73, "Douglas-fir forest (daytime)", "Giles et al., 1984" },
	{ 0.72, "Spruce forest (daytime)", "Shuttleworth and Calder, 1979" },
	{ 0.65, "Temperate coniferous", "Komatsu, 2005" },
	{ 0.63, "Coniferous", "Komatsu, 2005" },
	{ 0.55, "Boreal coniferous", "Komatsu, 2005" }
};

const bool CPriestleyTaylorET::AUTO_REGISTER = CETFactory::Register("Priestley-Taylor", &CPriestleyTaylorET::Create);
CPriestleyTaylorET::CPriestleyTaylorET()
{
	m_α = 1.26;//original value
}


ERMsg CPriestleyTaylorET::SetOptions(const CETOptions& options)
{
	ERMsg msg;
	
	if (options.OptionExist("PriestleyTaylorAlpha"))
	{
		size_t pos = as<size_t>(options.GetOption("PriestleyTaylorAlpha"));
		if (pos<NB_ALPHA)
			m_α = PRE_DEFINE_ALHA[pos].m_α;
	}
		

	return msg;
}


void CPriestleyTaylorET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);
	
	CTPeriod p = weather.GetEntireTPeriod();
	double Fcd = 0.6;//default value in Exel file
	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{
		
		const CDataInterface& data = weather[TRef];

		double λ = data.GetVarEx(H_LHVW)[MEAN];		// latent heat of vaporization [MJ kg-1]
		double Δ = data.GetVarEx(H_SSVP)[MEAN];		// slope of the saturation vapour pressure-temperature relationship [kPa °C-1]
		double ɣ = data.GetVarEx(H_PSYC)[MEAN];		// psychrometric constant [kPa °C-1]
		double Rn = max(0.0, data.GetNetRadiation(Fcd));		// net radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
		//double U² = data[H_WND2][MEAN] * 1000 / 3600;	//Wind speed at 2 meters [m/s]
		double G = WBSF::G(Rn);						// soil heat flux energie [MJ m-2 d-1] or [MJ m-2 hr-1]

		double Eє = (Δ / (Δ + ɣ)) * ((Rn - G) / λ);	// equilibrium evapotranspiration rate [kg m-2 d-1] or [kg m-2 hr-1]
		double ETo = m_α*Eє;						// evapotranspiration [kg m-2 day-1] or [kg m-2 h-1]

		if (TRef - p.Begin()>180)
		{
			int gg;
			gg = 0;
		}
		output[TRef][S_ET] = ETo;
	}
}


//
//// alt: elevation (m)
//// atm_pres: atmospheric pressure (pa)
//double CPriestleyTaylorET::atm_pres(double alt)
//{
//	ASSERT( alt!=-999);
//	// daily atmospheric pressure (Pa) as a function of elevation (m) 
//	// From the discussion on atmospheric statics in:
//	// Iribane, J.V., and W.L. Godson, 1981. Atmospheric Thermodynamics, 2nd
//	// Edition. D. Reidel Publishing Company, Dordrecht, The Netherlands. (p. 168)
//	
//	const double MA       =28.9644e-3;      // (kg mol-1) molecular weight of air 
//	const double R        =8.3143;          // (m3 Pa mol-1 K-1) gas law constant 
//    const double LR_STD   =0.0065;          // (-K m-1) standard temperature lapse rate 
//	const double G_STD    =9.80665;         // (m s-2) standard gravitational accel.  
//	const double P_STD    =101325.0;        // (Pa) standard pressure at 0.0 m elevation 
//	const double T_STD    =288.15;          // (K) standard temp at 0.0 m elevation   
//
//	double t1 = 1.0 - (LR_STD * alt)/T_STD;
//	double t2 = G_STD / (LR_STD * (R / MA));
//	double pa = P_STD * pow(t1,t2);
//	
//	return(pa);
//}



//*************************************************************
//CModifiedPriestleyTaylorET
//after Antonio Steidle Neto (2015)

const bool CSimplifiedPriestleyTaylorET::AUTO_REGISTER = CETFactory::Register("Simplified Priestley-Taylor", &CSimplifiedPriestleyTaylorET::Create);
CSimplifiedPriestleyTaylorET::CSimplifiedPriestleyTaylorET()
{
}




void CSimplifiedPriestleyTaylorET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);

	CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{

		//const CDataInterface& data = weather[TRef];
		const CWeatherDay& data = weather.GetDay(TRef);

		double RsRa = 0.16*sqrt(data[H_TMAX2][MEAN] - data[H_TMIN2][MEAN]);					// Hargreaves and 0.16 recommended by Allen et all (1998)
		double Ra = data.GetVarEx(H_EXRA)[MEAN];		//extraterrestrial radiation  [MJ/(m²·d)]
		double Rs = RsRa*Ra;							//Solar radiation  [MJ/(m²·d)]
		double rad = Rs * 1000000 / (3600*24);			//daylight radiation [W/m²]

		double dayl = data.GetDayLength();				//[s]
		double ta = data.GetTdaylight();				//[°C]
		double pa = WBSF::GetPressure(weather.m_alt);	//air pressure [Pa]

		double ETo = max(0.0, calc_pet(rad, ta, pa, dayl));

		CTPeriod p = data.GetEntireTPeriod();
		size_t nbTRef = p.size();
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			output[TRef][S_ET] = ETo / nbTRef;
	}
}



// calc_pet calculates the potential evapotranspiration for aridity
// corrections in calc_vpd(), according to Kimball et al., 1997 
//rad	:	daylight average incident shortwave radiation [W/m2]
//ta	:   daylight average air temperature [deg C]
//pa	:   air pressure [Pa]
//dayl	:   daylength [s]
double CSimplifiedPriestleyTaylorET::calc_pet(double rad, double ta, double pa, double dayl)
{
	// calculate absorbed radiation, assuming albedo = 0.2  and ground
	// heat flux = 10% of absorbed radiation during daylight 
	double rnet = rad * 0.72;	// absorbed shortwave radiation avail [W m-2]

	// calculate latent heat of vaporization as a function of ta 
	double lhvap = 2.5023e6 - 2430.54 * ta;	// latent heat of vaporization of water [J kg-1]

	// calculate the psychrometer parameter: gamma = (cp pa)/(lhvap epsilon)
	// where:
	// CP	:	specific heat of air (J/kg K)
	// EPS	:	ratio of molecular weights of water and air [unitless]
	static const double CP = 1010.0;		// specific heat of air [J kg-1 K-1]
	static const double EPS = 0.62196351;   // ratio of molec weights (MW/MA) [unitless}
	double ɣ = CP * pa / (lhvap * EPS);	// psychrometer parameter [Pa K-1]

	// estimate the slope of the saturation vapor pressure curve at ta 
	// temperature offsets for slope estimate 
	static const double DT = 0.2;   // offset for saturation vapor pressure calculation
	double t1 = ta + DT;			// air temperatures [°C]
	double t2 = ta - DT;			// air temperatures [°C]

	// calculate saturation vapor pressures at t1 and t2, using formula from
	// Abbott, P.F., and R.C. Tabony, 1985. The estimation of humidity parameters.
	// Meteorol. Mag., 114:49-56.
	double pvs1 = 610.7 * exp(17.38 * t1 / (239.0 + t1));	// saturated vapor pressures [Pa]
	double pvs2 = 610.7 * exp(17.38 * t2 / (239.0 + t2));	// saturated vapor pressures [Pa]

	// calculate slope of pvs vs. T curve near ta 
	double Δ = (pvs1 - pvs2) / (t1 - t2);					// slope of saturated vapor pressure curve [Pa K-1] 

	// calculate ET using Priestley-Taylor approximation, with coefficient
	// set at 1.26. Units of result are kg/m²/day, equivalent to mm water/day 
	double ET = (1.26 * (Δ / (Δ + ɣ)) * rnet * dayl) / lhvap;	// potential evapotranspiration [kg m-2 day-1]

	// return a value in mm/day, because this value is used in a ratio
	// to annual total precip, and precip units are centimeters 
	return ET;
}

//*************************************************************
//CModifiedPriestleyTaylorET

const bool CModifiedPriestleyTaylorET::AUTO_REGISTER = CETFactory::Register("Modified Priestley-Taylor", &CModifiedPriestleyTaylorET::Create);
CModifiedPriestleyTaylorET::CModifiedPriestleyTaylorET()
{
	m_method = HOLTSLAG_VAN_ULDEN;
}


ERMsg CModifiedPriestleyTaylorET::SetOptions(const CETOptions& options)
{
	ERMsg msg;
	
	if (options.OptionExist("ModifiedPriestleyTaylorMethod"))
		m_method = ToSizeT(options.GetOption("ModifiedPriestleyTaylorMethod"));

	return msg;
}

void CModifiedPriestleyTaylorET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	CTPeriod p = weather.GetEntireTPeriod();
	output.Init(p, NB_ET_STATS, 0, ET_HEADER);

	double Fcd = 0.6;//default value in Exel file
	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{
		const CDataInterface& data = weather[TRef];

		//The Priestley-Taylor coefficient (a, unitless), or coefficient of advectivity, represents the fraction of surface moisture available for evaporation in the Priestley-Taylor equation, either used as a constant input or as a function of some weather variables.
		//According to its original formulation (Priestley and Taylor, 1972), a is a constant term (a=PTc, where PTc is the dimensionless Priestley-Taylor constant). An average value of PTc=1.26 was found by the authors and theoretically explained by Lhomme (1996) for "the evapotranspiration from a horizontally uniform saturated surface", that closely resembles a surface of well-watered short grasses under humid conditions. The literature shows that PTc can vary from 1.08 to more than 1.60 as a function of the advectivity of the environment (Villalobos et al., 2002). The constant should be increased for arid and semi-arid climates up to PTc=1.70-1.75, according to ASCE (1990). Lower values are expected for wetlands.
		//The modified Priestley-Taylor coefficient by Holtslag and Van Ulden (1983) is:
		//
		static const double β = 0.6;		// is the ratio of sensible to latent heat flux(Bowen ratio). A default value is β = 0.6.
		static const double Cd = 0.34;

		//static const double λ = 2.45;		// latent heat of vaporization FAO-56 Penman-Monteith [MJ kg-1]
		double λ = data.GetVarEx(H_LHVW);		// latent heat of vaporization [MJ kg-1]
		double Δ = data.GetVarEx(H_SSVP);		// slope of the saturation vapour pressure-temperature relationship [kPa °C-1]
		double ɣ = data.GetVarEx(H_PSYC);		// psychrometric constant [kPa °C-1]
		double Rn = max(0.0, data.GetNetRadiation(Fcd));		// net radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
		//double G = data.GetVarEx(SHFE);		// soil heat flux energie [MJ m-2 d-1] or [MJ m-2 hr-1]
		double G = WBSF::G(Rn);						// soil heat flux energie [MJ m-2 d-1] or [MJ m-2 hr-1]
		double U² = data[H_WND2][MEAN]*1000/3600;	//Wind speed at 2 meters [m/s]
		double rᶜᵃ = Cd*U²;
		double Ω = 1 / (1 + (ɣ / (Δ + ɣ))*rᶜᵃ);	//Omega

		double α = 1.26;
		switch (m_method)
		{
		case HOLTSLAG_VAN_ULDEN: α = (1 + ɣ / Δ) / (1 + β);	break;	//The modified Priestley-Taylor coefficient by Holtslag and Van Ulden (1983)
		case ALPHA_OMEGA: α = (1 / Ω); break;					//The modified Priestley-Taylor coefficient by the Alpha-Omega method
		default:ASSERT(false);
		}
		
		double Eᴱ = (Δ / (Δ + ɣ))*((Rn - G) / λ);
		double ETo = α * Eᴱ;
		


		output[TRef][S_ET] = ETo;
		
		// λ is the latent heat of vaporation of water, in J/g,
		//E is the evaporation rate, in g/m2-s,
		//the product λ E is the latent heat flux, or energy used for evapotranspiration, in W/m2,
		//α is the Priestley-Taylor coefficient (dimensionless),
		//∆ is the slope of the saturation vapor-pressure curve, in kPa/°K,
		//A is the available energy (sum of net radiation,soil heat flux, and change in heat storage in water), in W/m2, and
		//γ is the psychrometric constant computed from atmospheric pressure and air temperature (Fritschen and Gay, 1979), in kPa/°C. 

	}
}



//**************************************************************************************

const bool CHamonET::AUTO_REGISTER = CETFactory::Register("Hamon", &CHamonET::Create);
//calibration variable???
const double CHamonET::KPEC = 1.2;

CHamonET::CHamonET()
{
}



//daily evapotranspiration
// see article: "A comparison of six potential evapotranspiration methods", Journal of American water resources Association 2005
void CHamonET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);
	
	for (size_t y = 0; y<weather.size(); y++)
	{
		for (size_t m = 0; m<weather[y].size(); m++)
		{
			for (size_t d = 0; d<weather[y][m].size(); d++)
			{
				const CDay& wDay = weather[y][m][d];
				
				double dailyET = 0;
				double T = wDay[H_TNTX][MEAN];
				if (T > 0)
				{
					double Ld = wDay.GetDayLength() / (60 * 60 * 12);//in multiple of 12 hours

					double ESAT = 6.208*exp(17.26939*T / (T + 237.3));
					double RHOSAT = 216.7*ESAT / (T + 273.3);
					dailyET = 0.1651*Ld*RHOSAT*KPEC;
				}

				//ET distributed over time
				CTPeriod p = weather[y][m][d].GetEntireTPeriod();
				size_t nbTRef = p.size();
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					output[TRef][S_ET] = dailyET / nbTRef;
			}
		}
	}
}


//**************************************************************************************

const bool CModifiedHamonET::AUTO_REGISTER = CETFactory::Register("ModifiedHamon", &CHamonET::Create);
CModifiedHamonET::CModifiedHamonET()
{}

ERMsg CModifiedHamonET::SetOptions(const CETOptions& options)
{
	ERMsg msg;
	return msg;
}

void CModifiedHamonET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);

	for (size_t y = 0; y < weather.size(); y++)
	{
		for (size_t m = 0; m < weather[y].size(); m++)
		{
			CStatistic T;
			CStatistic D;
			for (size_t d = 0; d < weather[y][m].size(); d++)
			{
				T += weather[y][m][d][H_TNTX][MEAN];
				D += weather[y][m][d].GetDayLength();
			}

			double monthlyET = 0;
			if (T[MEAN] > 0)
			{
				double Wt = 4.95*exp(0.062*T[MEAN]) / 100;
				monthlyET = 13.97*weather[y][m].size()*Square(D[MEAN] / (12 * 60 * 60))*Wt;
			}

			//ET distributed over time
			CTPeriod p = weather[y][m].GetEntireTPeriod();
			size_t nbTRef = p.size();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				output[TRef][S_ET] = monthlyET / nbTRef;

		}
	}
}


//*************************************************************
//Hargreaves original

//The history of development of the 1985 Hargreaves equation is described in Hargreaves et al.,2003. 
//Combining the two equations(5) and(6), the Hargreaves equation(7) for daily
//evapotranspiration can be found.The actual factor of 0.0022 was adjusted by Hargreaves to 0.0023 .
//(5)
//ET = 0.0135 * Rs*conv*(T+17.8)

//Evapotranspiration after Hargreaves[mm day-1]
//T		:	Mean temperature of the day[°C]
//Rs	:	Solar radiation[MJ m-2 day-1]
//conv	:	Conversion to ET equivalent (0.4082) [m2 mm MJ-1]
//
//Rs =0.16*Ra*(Tmax-Tmin)^0.5
//(6)
//Rs	:	Solar radiation[MJ m-2 day-1]
//Ra	:	Extraterrestrial radiation[MJ m-2 day-1]
//Tmax	:	Maximum temperature of the day[°C]
//Tmin	:	Minimum temperature of the day[°C]


const bool CHargreavesET::AUTO_REGISTER = CETFactory::Register("Hargreaves", &CHamonET::Create);
void CHargreavesET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);

	for (size_t y = 0; y < weather.size(); y++)
	{
		for (size_t m = 0; m < weather[y].size(); m++)
		{
			for (size_t d = 0; d < weather[y][m].size(); d++)
			{
				static const double C = 0.4082; //Conversion to ET equivalent [(m2 mm)/MJ]

				double Tmin = weather[y][m][d][H_TMIN2][MEAN];
				double T = weather[y][m][d][H_TNTX][MEAN];
				double Tmax = weather[y][m][d][H_TMAX2][MEAN];
				double Rs = weather[y][m][d][H_SRMJ][SUM];
				//double Rs = 0.16*Ra*(Tmax - Tmin) ^ 0.5;
				double dailyET = 0.0135 * Rs*C*(T + 17.8);
				
				//ET distributed over time
				CTPeriod p = weather[y][m][d].GetEntireTPeriod();
				size_t nbTRef = p.size();
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					output[TRef][S_ET] = dailyET / nbTRef;

			}
		}
	}
}



//*************************************************************
//Hargreaves-Samani
//from : http://www.zohrabsamani.com/research_material/files/Hargreaves-samani.pdf

const bool CHargreavesSamaniET::AUTO_REGISTER = CETFactory::Register("Hargreaves-Samani", &CHargreavesSamaniET::Create);
void CHargreavesSamaniET::Execute(const CWeatherStation& weather, CModelStatVector& output)
{
	output.Init(weather.GetEntireTPeriod(), NB_ET_STATS, 0, ET_HEADER);

	for (size_t y = 0; y < weather.size(); y++)
	{
		for (size_t m = 0; m < weather[y].size(); m++)
		{
			for (size_t d = 0; d < weather[y][m].size(); d++)
			{
				//static const double KT[2] = { 0.162, 0.190 }; //Hargreaves(1994) recommended using KT = 0.162 for "interior" regions and KT = 0.19 for coastal regions.
				static const double C = 0.4082; //Conversion to ET equivalent [(m²·mm)/MJ]
				
				double T = weather[y][m][d][H_TNTX][MEAN];
				double ΔT = weather[y][m][H_TRNG2][MEAN];//montlhy difference
				
				double KT = 0.00185*ΔT*ΔT - 0.0433*ΔT + 0.4023; //[°C-½] R² = 0.70, S.E. = 0.0126
				double Ra = C * weather[y][m][d][H_EXRA][SUM];	//[mm/d];
				double Rs = KT * Ra * sqrt(ΔT);

				
				//double KT = 0.162;
				double dailyET = max(0.0, 0.0135*Rs*(T + 17.8));

				//ET distributed over time
				CTPeriod p = weather[y][m][d].GetEntireTPeriod();
				size_t nbTRef = p.size();
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					output[TRef][S_ET] = dailyET / nbTRef;

			}
		}
	}
}

//*********************************************************************************************************************************
//Penman-Monteith

const bool CPenmanMonteithET::AUTO_REGISTER = CETFactory::Register("Penman-Monteith", &CPenmanMonteithET::Create);
CPenmanMonteithET::CPenmanMonteithET()
{}

void CPenmanMonteithET::Execute(const CWeatherStation& weather, CModelStatVector& stats)
{
	ASSERT(weather.m_lat >= -90 && weather.m_lat <= 90);

	CTPeriod p = weather.GetEntireTPeriod();
	stats.Init(p, NB_ET_STATS, 0, ET_HEADER);

	//altitude in meters
	double z = weather.m_alt;
	double Fcd = 0.6;//default value in Excel file
	double Bᵪ = 101.3*pow((293 - 0.0065*z) / 293, 5.25);

	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{
		const CDataInterface& data = weather[TRef];

		double Tmin = data[H_TMIN2][MEAN];
		double Tmax = data[H_TMAX2][MEAN];
		double T = data[H_TNTX][MEAN];
		double U² = data[H_WND2][MEAN] * 1000 / 3600; ASSERT(U² >= 0);	//Wind speed at 2 meters [m/s]
		double Ea = data[H_EA2][MEAN] / 1000;	//vapor pressure [kPa]
		double Es = data[H_ES2][MEAN] / 1000;	//vapor pressure [kPa]

		double Rn = max(0.0, data.GetNetRadiation(Fcd));
		double G = weather.IsHourly() ? CASCE_ETsz::GetGH(CASCE_ETsz::SHORT_REF, Rn) : 0;
		double nbSteps = weather.IsHourly() ? 24 : 1;

		double λ = 2.501 - (2.361*0.001)*T;
		double ɣ = 0.00163*Bᵪ / λ;
		//double Δ = (4099 * Es) / Square(T + 237.3);
		double Δ = data.GetVarEx(H_SSVP);
		//double Δɣ = Δ + ɣ;

		//double Rad = (1 / λ)*Δ*(Rn - G) / Δɣ;
		//double aero = ɣ*( (900/nbSteps) / (T + 273))*U²*(Es - Ea) / Δɣ;
		//double ETsz = Rad + aero;
		double num = 0.408*Δ*(Rn - G) + ɣ*((900 / nbSteps) / (T + 273))*U²*(Es - Ea);
		double dnom = Δ + ɣ*(1 + 0.34*U²);
		double ETo = num / dnom;
		stats[TRef][S_ET] = ETo;
	}

}

//*************************************************************
//CASCE_ETsz

//from http://www.kimberly.uidaho.edu/water/asceewri/ascestzdetmain2005.pdf

const bool CASCE_ETsz::AUTO_REGISTER = CETFactory::Register("ASCE2005", &CASCE_ETsz::Create);
//daily
const CASCE_ETsz::CCorrectionFactors CASCE_ETsz::CorrectionFactors[NB_REF] = 
{
	{ 900, 0.34},	//short
	{ 1600, 0.38 }	//tall
};
//hourly
const CASCE_ETsz::CCorrectionFactors CASCE_ETsz::CorrectionFactorsH[NB_REF][2] =
{
//     Daytime        nighttime
	{ { 37, 0.24 }, { 37, 0.96 } },	//short
	{ { 66, 0.25 }, { 66, 1.7 } }	//tall
	
};
//Lastent Heat of Vaporisation (MJ/kg)
//const double CASCE_ETsz::LHV = 2.45;

CASCE_ETsz::CASCE_ETsz(size_t referenceType, bool extended )
{
	m_referenceType = (TReference)referenceType;
	//m_bExtended = extended;
}

ERMsg CASCE_ETsz::SetOptions(const CETOptions& options)
{
	ERMsg msg;

	//if (options.OptionExist("ASCE_Extended"))
		//m_bExtended = ToBool(options.GetOption("ASCE_Extended"));

	if (options.OptionExist("ASCE_ReferenceType"))
		m_referenceType = (TReference)ToShort(options.GetOption("ASCE_ReferenceType"));

	if (options.OptionExist("ASCE_ETref"))
		m_referenceType = (TReference)ToShort(options.GetOption("ASCE_ETref"));

	return msg;
}

//this code was taken from the document:
//the ASCE standardized reference evapotranpiration equation
//unfortunately, because the wind speed isn't avalable
//these equations can't be used
//input :	weather: annual weather object
//			lat: latitude of the point(degree)
//			alt : elevation(m)
//			type : reference type( 0=SHORT_REF, 1=LONG_REF)
void CASCE_ETsz::Execute(const CWeatherStation& weather, CModelStatVector& stats)
{
	ASSERT(weather.m_lat >= -90 && weather.m_lat <= 90);
	ASSERT(m_referenceType >= 0 && m_referenceType < NB_REF);
	
	CTPeriod p = weather.GetEntireTPeriod();
	stats.Init(p, NB_ET_STATS, 0, ET_HEADER);
//	stats.Init(p, m_bExtended ? NB_EXTENDED_STATS:NB_ET_STATS, 0, EXTENDED_ET_HEADER);

	//altitude in meters
	double z = weather.m_alt;
	double Fcd = 0.6;//default value in Richard Allen Excel file

	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{
		const CDataInterface& data = weather[TRef];

		double T = data[H_TNTX][MEAN];
		double U² = data[H_WND2][MEAN] * 1000 / 3600; ASSERT(U² >= 0);//wind speed at 2 meters [m/s]
		double P = data[H_PRES][MEAN] / 10; ASSERT(!IsMissing(data[H_PRES][MEAN]));//pressure [kPa]
		double Ea = data[H_EA2][MEAN] / 1000;	ASSERT(!IsMissing(data[H_EA2][MEAN]));//vapor pressure [kPa]
		double Es = data[H_ES2][MEAN] / 1000;	ASSERT(!IsMissing(data[H_ES2][MEAN]));//vapor pressure [kPa]

//			double Ra = data.GetExtraterrestrialRadiation();
//		double Fcd² = WHour.GetCloudiness(Ra);
	//	if (Fcd² >= 0)
		//	Fcd = Fcd²;

		//double Rnl = WHour.GetNetLongWaveRadiation(Fcd);
		//double Rns = WHour.GetNetShortWaveRadiation();
		double Rn = max(0.0, data.GetNetRadiation(Fcd));
		double Δ = GetSlopeOfSaturationVaporPressure(T);
		double ɣ = GetPsychrometricConstant(P);

		//double λ = 2.501 - (2.361*0.001)*T;
		//constant take from the table 
		double Cn = weather.IsHourly() ? GetCnH(m_referenceType, Rn >= 0) : GetCn(m_referenceType);
		double Cd = weather.IsHourly() ? GetCdH(m_referenceType, Rn >= 0) : GetCd(m_referenceType);
		double G = weather.IsHourly() ? GetGH(m_referenceType, Rn) : 0;

		double ETsz = GetETsz(Rn, G, T, U², Ea, Es, Δ, ɣ, Cn, Cd);
		stats[TRef][S_ET] = ETsz;
		//double Bᵪ = 101.3*pow((293 - 0.0065*z) / 293, 5.25);
		//double ɣ2 = 0.00163*Bᵪ / λ;
		////double ɣ3 = data.GetVarEx(PSYC)[MEAN];		// psychrometric constant [kPa °C-1]
		//double rᶳ = Rn < 0 ? 200 : 50;
		//double rₐ = U² <= 0.5 ? 208 / 0.5 : 208 / U²;
		//double ɣ° = ɣ2*(1 + rᶳ / rₐ);
		//double Δ2 = (4099 * Es) / Square(T + 237.3);
		//double Δɣ° = Δ2 + ɣ°;
		//double ΔΔɣ° = Δ2 / Δɣ°;
		//double Rad = (1 / λ)*Δ2*(Rn - G) / Δɣ°;
		//double aero = ɣ*(Cn / (T + 273))*U²*(Es - Ea) / Δɣ°;
		//double ETsz2 = Rad + aero;
		//double LE = 2.45*ETsz2;
		//double H = Rn - G - LE;
		////double num = 0.408*Δ*(Rn - G) + γ*(Cn / (T + 273))*u2*(Es - Ea);
		//	//double dnom = Δ + γ*(1 + Cd*u2);
		//	//double ETsz = num / dnom;

		


		//stats[TRef][S_ET2] = ETsz2;
		//if (m_bExtended)
		//{
			
			// double Ra = WHour.GetExtraterrestrialRadiation();
			//	double Fcd² = WHour.GetCloudiness(Ra);
			//	if (Fcd² >= 0)
			//		Fcd = Fcd²;

			//	double Rnl = WHour.GetNetLongWaveRadiation(Fcd);
			//	double Rns = WHour.GetNetShortWaveRadiation();
			//	double Rn = GetNetRadiation(Rnl, Rns);
			//	double Δ = GetSlopeOfSaturationVaporPressure(T);
			//	
			//	double ɣ = GetPsychrometricConstant(P);

			//	double λ = 2.501 - (2.361*0.001)*T;
			//	double Bᵪ = 101.3*pow((293 - 0.0065*z) / 293, 5.25);
			//	double ɣ2 = 0.00163*Bᵪ / λ;

			//	double ɣ3 = WHour.GetVarEx(PSYC)[MEAN];		// psychrometric constant [kPa °C-1]
			//	
			//	//constant take from the table 
			//	double Cn = GetCnH(m_referenceType, Rn >= 0);
			//	double Cd = GetCdH(m_referenceType, Rn >= 0);
			//	double G = GetG(m_referenceType, Rn);

			//	double ETsz = GetETsz(Rn, G, T, U2, Ea, Es, Δ, ɣ, Cn, Cd);
			//	stats[TRef][S_ET] = ETsz;

			//	double rₐ = Rn < 0 ? 200 : 50;
			//	double rᶳ = U2 <= 0.5 ? 208 / 0.5 : 208 / U2;
			//	double ɣ° = ɣ2*(1 + rₐ / rᶳ);
			//	double Δ2 = (4099 * Ea) / Square(T + 237.3);
			//	double Δɣ° = Δ2 + ɣ°;
			//	//double ΔΔɣ° = Δ2 / Δɣ°;
			//	double Rad = (1 / λ)*Δ2*(Rn - G) / Δɣ°;
			//	double aero = ɣ*(Cn / (T + 273))*U2*(Es - Ea) / Δɣ°;
			//	double ETsz2 = Rad + aero;
			//	double LE = 2.45*ETsz2;
			//	double H = Rn-G-LE;
			//	//double num = 0.408*Δ*(Rn - G) + γ*(Cn / (T + 273))*u2*(Es - Ea);
			//	//double dnom = Δ + γ*(1 + Cd*u2);
			//	//double ETsz = num / dnom;


			//	//G	H	LE	dr	d	Sc	w	w1	w2	sin q	Ra	b	Rso	Rs	ea	ed	e'	Rs/Rso	f	Rns	Rnl	Rn	l	g	G	rs	ra	g*	D	D+g*	D/D+g*	Rad	Aero	LE	H
			//stats[TRef][S_RA] = Ra;
			//stats[TRef][S_RSO] = WHour.GetClearSkySolarRadiation(stats[TRef][S_RA]);
			//stats[TRef][S_FCD] = Fcd;
			//stats[TRef][S_RNL] = Rnl;
			//stats[TRef][S_RNS] = Rns;
			//stats[TRef][S_RN] = Rn;
		//}
		//if (weather.IsHourly())
		//{
		//	const CHourlyData& WHour = weather.GetHour(TRef);
		//	
		//	double T = WHour[H_TAIR];
		//	double U2 = WHour[H_WND2] * 1000 / 3600;ASSERT(U2 >= 0);//wind speed at 2 meters [m/s]
		//	double P = WHour[H_PRES] / 10; ASSERT(!IsMissing(P));//pressure [kPa]
		//	double Ea = WHour[H_EA];
		//	double Es = WHour[H_ES];

		//	double Ra = WHour.GetExtraterrestrialRadiation();
		//	double Fcd² = WHour.GetCloudiness(Ra);
		//	if (Fcd² >= 0)
		//		Fcd = Fcd²;

		//	double Rnl = WHour.GetNetLongWaveRadiation(Fcd);
		//	double Rns = WHour.GetNetShortWaveRadiation();
		//	double Rn = GetNetRadiation(Rnl, Rns);
		//	double Δ = GetSlopeOfSaturationVaporPressure(T);
		//	
		//	double ɣ = GetPsychrometricConstant(P);

		//	double λ = 2.501 - (2.361*0.001)*T;
		//	double Bᵪ = 101.3*pow((293 - 0.0065*z) / 293, 5.25);
		//	double ɣ2 = 0.00163*Bᵪ / λ;

		//	double ɣ3 = WHour.GetVarEx(PSYC)[MEAN];		// psychrometric constant [kPa °C-1]
		//	
		//	//constant take from the table 
		//	double Cn = GetCnH(m_referenceType, Rn >= 0);
		//	double Cd = GetCdH(m_referenceType, Rn >= 0);
		//	double G = GetG(m_referenceType, Rn);

		//	double ETsz = GetETsz(Rn, G, T, U2, Ea, Es, Δ, ɣ, Cn, Cd);
		//	stats[TRef][S_ET] = ETsz;

		//	double rₐ = Rn < 0 ? 200 : 50;
		//	double rᶳ = U2 <= 0.5 ? 208 / 0.5 : 208 / U2;
		//	double ɣ° = ɣ2*(1 + rₐ / rᶳ);
		//	double Δ2 = (4099 * Ea) / Square(T + 237.3);
		//	double Δɣ° = Δ2 + ɣ°;
		//	//double ΔΔɣ° = Δ2 / Δɣ°;
		//	double Rad = (1 / λ)*Δ2*(Rn - G) / Δɣ°;
		//	double aero = ɣ*(Cn / (T + 273))*U2*(Es - Ea) / Δɣ°;
		//	double ETsz2 = Rad + aero;
		//	double LE = 2.45*ETsz2;
		//	double H = Rn-G-LE;
		//	//double num = 0.408*Δ*(Rn - G) + γ*(Cn / (T + 273))*u2*(Es - Ea);
		//	//double dnom = Δ + γ*(1 + Cd*u2);
		//	//double ETsz = num / dnom;


		//	stats[TRef][S_ET2] = ETsz2;
		//	if (m_bExtended)
		//	{
		//		stats[TRef][S_RA] = Ra;
		//		stats[TRef][S_RSO] = WHour.GetClearSkySolarRadiation(stats[TRef][S_RA]);
		//		stats[TRef][S_FCD] = Fcd;
		//		stats[TRef][S_RNL] = Rnl; 
		//		stats[TRef][S_RNS] = Rns;
		//		stats[TRef][S_RN] = Rn;
		//	}
		//}
		//else
		//{
		//	const CWeatherDay& Wday = weather.GetDay(TRef);
		//	
		//	int J = int(TRef.GetJDay()) + 1;//Julian day

		//	//soil heat flux density at the soil surface( MJ m-2 d-1)
		//	const double G = 0;
		//	double U2 = Wday[H_WND2][MEAN] * 1000 / 3600;//wind speed in m/s at 2 meters;
		//	ASSERT(U2 >= 0);
		//	//if (IsMissing(u2))
		//	//{
		//	//	double u10 = Wday[H_WNDS][MEAN] * 1000 / 3600;//wind speed in m/s at 10 meters
		//	//	ASSERT(!IsMissing(u10));
		//	//	u2 = GetWindProfileRelationship(u10, 10);
		//	//}

		//	double T = GetMeanAirTemperature(Wday[H_TAIR][LOWEST], Wday[H_TAIR][HIGHEST]);
		//	//double u2 = GetWindProfileRelationship(u10, 10);
		//	double Ea = GetActualVaporPressure(Wday[H_TDEW]);//Wday.GetEa();
		//	//double Ea = GetActualVaporPressure(Wday[H_RELH][MEAN], Wday[H_TAIR][LOWEST], Wday[H_TAIR][HIGHEST]);//Wday.GetEa();
		//	double Es = GetSaturationVaporPressure(Wday[H_TAIR][LOWEST], Wday[H_TAIR][HIGHEST]);//Wday.GetEs();
		//	double Δ = GetSlopeOfSaturationVaporPressure(T);
		//	double P = GetAtmosphericPressure(z);
		//	//double P = Wday[H_PRES];
		//	//if (IsMissing(P))
		//		//P = GetAtmosphericPressure(z);
		//	double γ = GetPsychrometricConstant(P);
		//	
		//	double Rs = Wday[H_SRAD];
		//	double Ra = GetExtraterrestrialRadiation(weather.m_lat, J);
		//	double Rso = GetClearSkySolarRadiation(Ra, z);
		//	double Fcd = GetCloudinessFunction(Rs, Rso);
		//	double Rns = GetNetShortWaveRadiation(Rs);
		//	double Rnl = GetNetLongWaveRadiation(Wday[H_TAIR][LOWEST], Wday[H_TAIR][HIGHEST], Ea, Fcd);
		//	double Rn = GetNetRadiation(Rns, Rnl);

		//	//constant take from the table 
		//	double Cn = GetCn(m_referenceType);
		//	double Cd = GetCd(m_referenceType);

		//	double ETsz = GetETsz(Rn, G, T, U2, Ea, Es, Δ, γ, Cn, Cd);
		//	stats[TRef][S_ET] = ETsz;
		//	//stats[TRef][HOURLY_ARIDITY] = max(0.0, Wday[H_PRCP] - ETsz);
		//	if (m_bExtended)
		//	{
		//		stats[TRef][S_RA] = Ra;
		//		stats[TRef][S_RSO] = Rso;
		//		stats[TRef][S_FCD] = Fcd;
		//		stats[TRef][S_RNL] = Rnl;
		//		stats[TRef][S_RNS] = Rns;
		//		stats[TRef][S_RN] = Rn;
		//	}
		//}
	}
}


//	Rn[In]: calculated net radiation at the crop surface (MJ m-2 d-1for daily time steps or MJ m-2 h-1 for hourly time steps),
//	G[In]: soil heat flux density at the soil surface (MJ m-2 d-1 for daily time steps or MJ m-2 h-1 for hourly time steps),
//	T[In]: mean daily or hourly air temperature at 1.5 to 2.5-m height (°C),
//	u2[In]: mean daily or hourly wind speed at 2-m height (m s-1),
//	es[In]: saturation vapor pressure at 1.5 to 2.5-m height (kPa), calculated for daily time steps as the average of saturation vapor pressure at maximum and minimum air temperature,
//	ea[In]: mean actual vapor pressure at 1.5 to 2.5-m height (kPa),
//	Δ[In]: slope of the saturation vapor pressure-temperature curve (kPa °C-1),
//	γ[In]: psychrometric constant (kPa °C-1),
//	Cn[In]: numerator constant that changes with reference type and calculation time step (K mm s3 Mg-1 d-1 or K mm s3 Mg-1 h-1) and
//	Cd[In]: denominator constant that changes with reference type and calculation time step (s m-1).
//	ETsz[Out]: standardized reference crop evapotranspiration for short (ETos) or tall (ETrs) surfaces (mm d-1 for daily time steps or mm h-1 for hourly time steps),
double CASCE_ETsz::GetETsz(const double& Rn, const double& G, const double& T, const double& u2, const double& Ea, 
								const double& Es, const double& Δ, const double& γ, const double& Cn, const double& Cd)
{
	//[1]: The Standardized Reference Evapotranspiration Equation is intended to simplify and clarify 
	//the presentation and application of the method. As used in this report, 
	//the term ETsz refers to both ETos and ETrs. Eq. 1 presents the form of the Standardized Reference Evapotranspiration Equation
	//Units for the 0.408 coefficient are m2 mm MJ-1.

	double num = 0.408*Δ*(Rn-G)+γ*(Cn/(T+273))*u2*(Es-Ea);
	double dnom = Δ+γ*(1+Cd*u2);
	double ETsz = num/dnom;

	return ETsz;
}

//	Tmin [In]: daily minimum air temperature [°C]
//	Tmax [In]: daily maximum air temperature [°C]
//	T [out]: daily mean air temperature [°C]
double CASCE_ETsz::GetMeanAirTemperature (double Tmin, double Tmax)
{
	//[2]: For the standardized method, the mean air temperature, T, for a daily time step 
	//is preferred as the mean of the daily maximum and daily minimum air temperatures 
	//rather than as the average of hourly temperature measurements to provide for consistency across all data sets
	double T = (Tmin+Tmax)/2;
	return T;
}


//	z [In]: weather site elevation above mean sea level [m].
//	P [Out]: mean atmospheric pressure at station elevation z [kPa].
double CASCE_ETsz::GetAtmosphericPressure(double z)
{
	//[3]: The mean atmospheric pressure at the weather site is predicted from site elevation using a simplified formulation of the Universal Gas Law [Burman et al. (1987)]
	double P = 101.3* pow( (293-0.0065*z)/293, 5.26);
	return P;
}

//	P [In]: has units of kPa. 
//	γ [oUT]: has units of kPa °C-1.
double CASCE_ETsz::GetPsychrometricConstant(double P)
{
	ASSERT(P > 50 && P<150);
	//[4]: The standardized application using λ = 2.45 MJ kg-1 results in a value for the psychrometric constant, γ, that is proportional to the mean atmospheric pressure
	double γ = 0.0006650 * P;
	return γ;
}



//	T [In]: daily mean air temperature [°C].
//	Δ [Out]: slope of the saturation vapor pressure-temperature curve [kPa °C-1]
double CASCE_ETsz::GetSlopeOfSaturationVaporPressure(double T)
{
	//[5]: The slope of the saturation vapor pressure-temperature curve[Tetens (1930), Murray (1967)]
	double Δ = 2503* exp(17.27*T /(T+237.3))/Square(T+237.3); 
	return Δ;
}

//	T [In]: air temperature [°C].
//	e° [Out]: saturation vapor pressure function [kPa]
double CASCE_ETsz::e°(double T)
{
	//[6]: The saturation vapor pressure6 (es) represents the capacity of the air to hold water vapor.
	return 0.6108 * exp( 17.27*T/(T + 237.3));
}

//	Tmin [In]: daily minimum air temperature [°C]
//	Tmax [In]: daily maximum air temperature [°C]
//	Es[Out]: Saturation Vapor Pressure [kPa]
double CASCE_ETsz::GetSaturationVaporPressure( double Tmin, double Tmax)
{
	//[7]: The saturation vapor pressure [Jensen et al. (1990) and Tetens (1930)] (es) represents 
	//the capacity of the air to hold water vapor. 

	double Es = (e°(Tmax)+e°(Tmin))/2;
	return Es;
}

//  Tdew[In]: Measured or computed dew point temperature averaged over the daily period [°C]
//	Ea [Out]: Actual Vapor Pressure [kPa]
double CASCE_ETsz::GetActualVaporPressure(double Tdew)
{
	//[8]: The dew point temperature (Tdew) is the temperature to which the air must cool to reach a state of saturation. 
	//For daily calculation time steps, average dew point temperature can be computed by averaging over hourly periods.

	double Ea = e°(Tdew);
	return Ea;
}

//  RHmean[In]: Measured or computed relative humidity (%), 
//	Tmax[In]: maximum absolute temperature during the 24-hour period [°C] 
//	Tmin[In]: minimum absolute temperature during the 24-hour period [°C] 
//	Ea [Out]: Actual Vapor Pressure [kPa]
double CASCE_ETsz::GetActualVaporPressure(double RHmean, double Tmin, double Tmax)
{
	//[8]: The dew point temperature (Tdew) is the temperature to which the air must cool to reach a state of saturation. 
	//For daily calculation time steps, average dew point temperature can be computed by averaging over hourly periods.

	double Ea = (RHmean/100)*(e°(Tmin)+e°(Tmax))/2;
	return Ea;
}


//	Rns[In]: net short-wave radiation, [MJ m-2 d-1] (defined as being positive downwards and negative upwards)
//	Rnl[In]: net outgoing long-wave radiation, [MJ m-2 d-1] (defined as being positive upwards and negative downwards)
//	Rn [Out]: Net Radiation [MJ m-2 d-1]
double CASCE_ETsz::GetNetRadiation(double Rns, double Rnl)
{
	//[15]: Net radiation (Rn) is the net amount of radiant energy available at a vegetation 
	//or soil surface for evaporating water, heating the air, or heating the surface. 
	//Rn includes both short and long wave radiation components [Brutsaert (1982), Jensen et al., (1990), Wright (1982), Doorenbos and Pruitt (1975,1977), Allen et al., (1998).]
	double Rn = Rns - Rnl;				
	return Rn;
}


//	Rs[In]: incoming solar radiation [MJ m-2 d-1].
//	Rns[Out]: net solar or short-wave radiation [MJ m-2 d-1]
double CASCE_ETsz::GetNetShortWaveRadiation(double Rs)
{
	//[16]: Net short-wave radiation resulting from the balance between incoming and reflected solar radiation is given by
	//α: albedo or canopy reflection coefficient, is fixed at 0.23 for the standardized short and tall reference surfaces [dimensionless]
	static const double α = 0.23;
	double Rns = (1-α)*Rs;
	return Rns;
}



//	Ea[In]: actual vapor pressure [kPa],
//	Tmax[In]: maximum absolute temperature during the 24-hour period [°C] 
//	Tmin[In]: minimum absolute temperature during the 24-hour period [°C] 
//	Rnl[Out]: net long-wave radiation [MJ m-2 d-1],
double CASCE_ETsz::GetNetLongWaveRadiation(double Tmin, double Tmax, double Ea, double Fcd)
{
	//[17]: Rnl, net long-wave radiation, is the difference between upward long-wave radiation from the standardized 
	//surface (Rlu) and downward long-wave radiation from the sky (Rld), so that Rnl = Rlu – Rld. 
	//The following calculation for daily Rnl follows the method of Brunt (1932, 1952) of using vapor pressure to predict net emissivity:
	//The superscripts “4” in Eq. 17 indicate the need to raise the air temperature, expressed in Kelvin units, to the power of 4.
	//σ = Stefan-Boltzmann constant [4.901 x 10-9 MJ K-4 m-2 d-1]
	//fcd = cloudiness function [dimensionless] (limited to 0.05 ≤ fcd ≤ 1.0)
	//K = °C + 273.16

	Fcd = max( 0.05, min(1.0, Fcd ));

	const double Tkmin = Tmin + 273.16;
	const double Tkmax = Tmax + 273.16;

	//Stefan-Boltzman Constant ( MJ K-4 m-2 d-1)
	static const double σ = 4.901E-9;
	
	double Rnl = σ*Fcd*(0.34-0.14*sqrt(Ea))*((pow(Tkmax,4) + pow(Tkmin,4))/2 );
	return Rnl;
}

//	T[In]: maximum absolute temperature during [°C],
//	Ea[In]: actual vapor pressure [kPa],
//	Fcd[In]: minimum absolute temperature during [°C],
//	Rnl[Out]: net long-wave radiation [MJ m-2 h-1]
double CASCE_ETsz::GetNetLongWaveRadiationH(double T, double Ea, double Fcd)
{
	//[44]: Rnl is the difference between long-wave radiation radiated upward from 
	//the standardized surface (Rlu) and long-wave radiation radiated downward from 
	//the atmosphere (Rld), so that Rnl = Rlu – Rld. The following calculation for Rnl 
	//is the method introduced by Brunt (1932, 1952) that uses near surface vapor pressure 
	//to predict net surface emissivity.
	//The superscripts “4” in Eq. 17 indicate the need to raise the air temperature, expressed in Kelvin units, to the power of 4.
	//σ = Stefan-Boltzmann constant [2.042E-10 MJ K-4 m-2 h-1]
	//fcd = cloudiness function [dimensionless] (limited to 0.05 ≤ fcd ≤ 1.0)
	//K = °C + 273.16

	Fcd = max( 0.05, min(1.0, Fcd ));

	//Stefan-Boltzman Constant ( MJ K-4 m-2 h-1)
	static const double σ = 2.042E-10;
	
	double Tk = T + 273.16;
	double Є = (0.34 - 0.14*sqrt(Ea));
	double Rnl = σ*Fcd*Є*pow(Tk, 4);
	return Rnl;
}



//	Rs[In]: measured or calculated solar radiation [MJ m-2 d-1]
//	Rso[In]: calculated clear-sky radiation [MJ m-2 d-1].
//	Fcd [Out]: cloudines function [dimensionless] (limited to 0.05 ≤ fcd ≤ 1.0),
double CASCE_ETsz::GetCloudinessFunction (double Rs, double Rso)
{
//	[18]: The ratio Rs/Rso in Eq. 18 represents relative cloudiness and is limited to 0.3 < Rs/Rso ≤1.0 so that fcd has limits of 0.05 ≤ fcd ≤ 1.0.
//	Rs/Rso = relative solar radiation (limited to 0.3 ≤ Rs/Rso ≤ 1.0)

	double r = Rso!=0?Rs/Rso:1;
	double Fcd = max(0.3, min(1.0, 1.35*r - 0.35));
	ASSERT( Fcd >= 0.05 && Fcd<=1.0);

	return min(1.0, max(0.05, Fcd));
}

//Ra[Out]: extraterrestrial radiation for 1-Hour Periods [MJ m-2 h-1]
double CASCE_ETsz::GetExtraterrestrialRadiationH(CTRef TRef, double lat, double lon, double alt)
{
	//double Ra = -999;
	double Ra = 0;

	int h = (int)TRef.GetHour();

	//Julian day, in one base
	int J = int(TRef.GetJDay()) + 1;
	//mid hour of the period
	double t = h + 0.5;
	//latitude [radians]
	double ϕ = Deg2Rad(lat);
	//longitude of the solar radiation measurement site [expressed as positive degrees west of Greenwich, England]
	double Lm = CASCE_ETsz::GetLm(lon);
	//longitude of the center of the local time zone [expressed as positive degrees west of Greenwich, England]
	double Lz = CASCE_ETsz::GetCenterLocalTimeZone(Lm);
	//seasonal correction for solar time [hour]
	double Sc = CASCE_ETsz::GetSeasonalCorrection(J);
	//solar time angle at the midpoint of the period [radians]
	double ω = CASCE_ETsz::GetMidSolarTimeAngle(t, Lz, Lm, Sc);
	//solar declination [radians].
	double δ = CASCE_ETsz::GetSolarDeclination(J);
	double β = CASCE_ETsz::GetAngleOfSunAboveHorizon(ϕ, δ, ω);
	if (β >= 0.3)//~17.2°
	{
		//[inverse relative distance factor (squared) for the earth-sun [unitless]
		double dr = CASCE_ETsz::GetInverseRelativeDistanceFactor(J);
		//	sunset hour angle [radians]
		double ωs = CASCE_ETsz::GetSunsetHourAngle(ϕ, δ);
		//solar time angle at beginning of period [radians]
		double ω1 = CASCE_ETsz::GetBeginSolarTimeAngle(ω);
		//solar time angle at end of period [radians].
		double ω2 = CASCE_ETsz::GetEndSolarTimeAngle(ω);
		CASCE_ETsz::AdjustSolarTimeAngle(ω1, ω2, ωs);

		Ra = CASCE_ETsz::GetExtraterrestrialRadiationH(ϕ, δ, dr, ω1, ω2);
	}

	return Ra;
}

//double CASCE_ETsz::GetCloudiness(double Rs, double Rso)
//{
//	double Fcd = -999;
//	
//	if (Ra >= 0)
//	{
//		//double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, alt);
//		Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);
//	}
//
//	return Fcd;
//}

//	Ra[In]: Extraterrestrial Radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
//	z[In]: station elevation above sea level [m].
//	Rso[Out]: clear-sky radiation [MJ m-2 d-1] or [MJ m-2 hr-1].
double CASCE_ETsz::GetClearSkySolarRadiation (double Ra, double z)
{
	//[19]: When a dependable, locally calibrated procedure for determining Rso is not available, 
	//Rso, for purposes of calculating Rn, can be computed as:
	double Rso = (0.75 + 2E-5*z)*max(0.0, Ra);
	return Rso;
}


//	lat[In]: The latitude, lat, is positive for the Northern Hemisphere and negative for the Southern Hemisphere. 
//	J[In]: J is the number of the day in the year between 1 (1 January) and 365 or 366 (31 December).
//	Ra[Out] = extraterrestrial radiation for 24-Hour Periods [MJ m-2 d-1]
double CASCE_ETsz::GetExtraterrestrialRadiation(double lat, int J)
{
	ASSERT(J>=1&&J<=366);
	//[21]: Extraterrestrial radiation, Ra, defined as the short-wave solar radiation in the absence of an atmosphere,
	// is a well-behaved function of the day of the year, time of day, and latitude. 
	//It is needed for calculating Rso, which is in turn used in calculating Rn. 
	//For daily (24-hour) periods, Ra can be estimated from the solar constant, 
	//the solar declination, and the day of the year

	static const double Gsc = 4.92;// solar constant [MJ m-2 h-1]
	
	//latitude [radians]:
	double ϕ = Deg2Rad(lat);
	//[inverse relative distance factor (squared) for the earth-sun [unitless]
	double dr = GetInverseRelativeDistanceFactor(J);
	//solar declination [radians].
	double δ = GetSolarDeclination(J);
	//	sunset hour angle [radians].
	double ωs = GetSunsetHourAngle(ϕ, δ );
	
	double Ra = (24/π)*Gsc*dr*(ωs*sin(ϕ)*sin(δ)+cos(ϕ)*cos(δ)*sin(ωs));
	return Ra;
}

//The seasonal correction for solar time is:
//J[In]: J is the number of the day in the year between 1 (1 January) and 365 or 366 (31 December).
//Sc[OUT]: seasonal correction for solar time [hour].
double CASCE_ETsz::GetSeasonalCorrection(double J)
{
	//[58]
	double b = 2*π*(J-81)/364;
		
	//[57]:
	double Sc= 0.1645*sin(2*b) - 0.1255*cos(b) - 0.025*sin(b);
	
	return Sc;
}


//The solar time angles at the beginning and end of each period are given by
double CASCE_ETsz::GetBeginSolarTimeAngle(double ω)
{
	static const double t1 = 1;//length of the calculation period [hour]: i.e., 1 for hourly periods or 0.5 for 30-minute periods.
	
	//[53]: The solar time angles at the beginning 
	double ω1 = ω-π*t1/24;
	return ω1;
}

//The solar time angles at the beginning and end of each period are given by
double CASCE_ETsz::GetEndSolarTimeAngle(double ω)
{
	static const double t1 = 1;//length of the calculation period [hour]: i.e., 1 for hourly periods or 0.5 for 30-minute periods.
	//[54]: //The solar time angles at the end 
	double ω2 = ω+π*t1/24;
	return ω2;
}

//t[IN]: standard clock time at the midpoint of the period [hour] (after correcting time for any daylight savings shift). For example for a period between 1400 and 1500 hours, t = 14.5 hours,
//Lz[IN]: longitude of the center of the local time zone [expressed as positive degrees west of Greenwich, England]. In the United States, Lz = 75, 90, 105 and 120° for the Eastern, Central, Rocky Mountain and Pacific time zones, respectively, and Lz = 0° for Greenwich, 345° for Paris (France), and 255° for Bangkok (Thailand),
//Lm[IN]: longitude of the solar radiation measurement site [expressed as positive degrees west of Greenwich, England], and
//Sc[IN]: seasonal correction for solar time [hour].
//ω[OUT]: The solar time angle at the midpoint of the period [radians]
double CASCE_ETsz::GetMidSolarTimeAngle(double t, double Lz, double Lm, double Sc)
{
	//[55]: The solar time angle at the midpoint of the period
	double ω = π/12*((t+0.06667*(Lz-Lm)+Sc)-12);
	return ω;
}
	
void CASCE_ETsz::AdjustSolarTimeAngle(double& ω1, double& ω2, double ωs)
{
	//[56]
	if(ω1 < -ωs)
		ω1 = -ωs;
	if(ω2 < -ωs)
		ω2 = -ωs;
	if(ω1 > ωs)
		ω1 = ωs;
	if(ω2 > ωs)
		ω2 = ωs;
	if(ω1 > ω2)
		ω1 = ω2;
}

//longitude of the center of the local time zone [expressed as positive degrees west of Greenwich, England]. 
//In the United States, Lz = 75, 90, 105 and 120° for the Eastern, Central, Rocky Mountain and Pacific time zones, respectively, and Lz = 0° for Greenwich, 345° for Paris (France), and 255° for Bangkok (Thailand)
double CASCE_ETsz::GetCenterLocalTimeZone(double Lm)
{
	ASSERT(Lm>=0 && Lm<360); 
	int zone = Round(Lm/15)%24;
	ASSERT(zone>=0 && zone<24);
	
	return zone*15;
}

//lon[IN]: longitude in degree east
//Lm[OUT]: longitude of the solar radiation measurement site [expressed as positive degrees west of Greenwich, England]
double CASCE_ETsz::GetLm(double lon)
{
	ASSERT(lon>=-180 && lon<=180);

	double Lm = lon>=0?180+lon:-lon;
	return Lm;
}

//ϕ[IN]: latitude [radians],
//δ[IN]: solar declination [radians],
//ω[IN]: solar time angle at the midpoint of the period [radians] (from Eq. 55).
//β[OUT]: angle of the sun above the horizon at midpoint of the period [radians],
double CASCE_ETsz::GetAngleOfSunAboveHorizon(double ϕ, double δ, double ω)
{
	//[62]: The angle of the sun above the horizon, β, at the midpoint of the hourly or shorter time period is computed as:
	double β = asin( sin(ϕ)*sin(δ)+cos(ϕ)*cos(δ)*cos(ω) );
	return max(0.0, β);
}

//ϕ[IN]: latitude in radians
//δ[IN]: solar declination [radians].
//dr[IN]: inverse relative distance factor (squared) for the earth-sun [unitless]
//ω1[IN]: solar time angle at beginning of period [radians]
//ω2[IN]: solar time angle at end of period [radians].
//Ra[Out]: extraterrestrial radiation for 1-Hour Periods [MJ m-2 h-1]
double CASCE_ETsz::GetExtraterrestrialRadiationH(double ϕ,double δ,double dr,double ω1,double ω2)
{
	//[21]: Extraterrestrial radiation, Ra, defined as the short-wave solar radiation in the absence of an atmosphere,
	// is a well-behaved function of the day of the year, time of day, and latitude. 
	//It is needed for calculating Rso, which is in turn used in calculating Rn. 
	//For daily (24-hour) periods, Ra can be estimated from the solar constant, 
	//the solar declination, and the day of the year

	static const double Gsc = 4.92;// solar constant [MJ m-2 h-1]
	double ϴ = ((ω2 - ω1)*sin(ϕ)*sin(δ) + cos(ϕ)*cos(δ)*(sin(ω2) - sin(ω1)));
	double Ra = (12 / π)*Gsc*dr*ϴ;

	return Ra;
}

//	J[In]: J is the number of the day in the year between 1 (1 January) and 365 or 366 (31 December).
//	dr[Out]: inverse relative distance factor (squared) for the earth-sun [unitless]
double CASCE_ETsz::GetInverseRelativeDistanceFactor(int J)
{
	//The constant 365 in Eqs. 23 is held at 365 even during a leap year.
	//[23]: inverse relative distance factor (squared) for the earth-sun [unitless]
	double dr = 1 + 0.033 * cos( 2*π*J/365 );
	return dr;
}

//	J[In]: J is the number of the day in the year between 1 (1 January) and 365 or 366 (31 December).
//	δ[Out]: solar declination [radians].
double CASCE_ETsz::GetSolarDeclination(int J)
{
	//The constant 365 in Eqs. 23 and 24 is held at 365 even during a leap year.
	//[24]: solar declination [radians].
	double δ = 0.409* sin(2*π*J/365-1.39);
	return δ;
}

//	δ[In]: solar declination [radians].
//	ϕ[In]: latitude [radians].
//	ωs[Out]: sunset hour angle [radians].
double CASCE_ETsz::GetSunsetHourAngle(double ϕ, double δ )
{
	//[27]: The sunset hour angle, ωs, is given by:
	double a = max(-1.0, min(1.0, -tan(ϕ)*tan(δ)));
	double ωs = acos(a);
	return ωs;
}



//	uz[In]: measured wind speed at zw m above ground surface [m s-1], and
//	zw[In]: height of wind measurement above ground surface [m].
//	u2[Out]: wind speed at 2 m above ground surface [m s-1],
double CASCE_ETsz::GetWindProfileRelationship(double uz, double zw)
{
	//[33]: Wind speed varies with height above the ground surface. For the calculation of ETsz, wind speed at 2 meters above the surface is required, therefore, wind measured at other heights must be adjusted. To adjust wind speed data to the 2-m height, Eq. 33 should be used for measurements taken above a short grass (or similar) surface, based on the full logarithmic wind speed profile equation B.14 given in Appendix B:
	double u2 = uz*4.87/log(67.8*zw-5.42);
	return u2;
}

double CASCE_ETsz::GetGH(int type, double Rn)
{
	static const double G[2][2] = { { 0.1, 0.5 }, { 0.04, 0.2 } };
	
	bool bDaytime = Rn >= 0;
	return G[type][bDaytime ? 0 : 1] * Rn;
	//For the standardized short reference ETos :
	//For the standardized tall reference ETrs :
	//where G and Rn have the same measurement units (MJ m-2 h-1 for hourly or shorter time periods).
	
	
}



#ifdef _DEBUG

void CASCE_ETsz::UnitTest()
{
	//test take from the appendices C
	double Tmin=10.9;
	double Tmax=32.4;
	double Ea=1.27;
	double Es=3.09;
	double Rs=22.4;
	double u2=1.79;
	double lat=40.41;
	double z=1462.4;
	int J=183;

	//soil heat flux density at the soil surface( MJ m-2 d-1)
	const double G = 0;
	double T = GetMeanAirTemperature(Tmin, Tmax);
	double Δ = GetSlopeOfSaturationVaporPressure(T);
	double P = GetAtmosphericPressure(z);
	double γ = GetPsychrometricConstant(P);

	double Ra = GetExtraterrestrialRadiation(lat, J);
	double Rso = GetClearSkySolarRadiation (Ra, z);
	double Fcd = GetCloudinessFunction (Rs, Rso);
	double Rns = GetNetShortWaveRadiation(Rs);
	double Rnl = GetNetLongWaveRadiation(Tmin, Tmax, Ea, Fcd);
	double Rn = GetNetRadiation(Rns, Rnl);
		

	double pet = CASCE_ETsz::GetETsz(Rn, G, T, u2, Ea, Es, Δ, γ, GetCn(0), GetCd(0));
	_ASSERTE( fabs(pet -5.71) < 0.01);
		//constant take from the table 
	pet = CASCE_ETsz::GetETsz(Rn, G, T, u2, Ea, Es, Δ, γ, GetCn(1), GetCd(1));
	_ASSERTE( fabs(pet -7.34) < 0.01);



}

#endif



//*******************************************************************************************************
//Crop coeficient from AIMM
//http://agriculture.alberta.ca/acis/imcin/aimm.jsp
const double CKropCoeficient::KROP_COEFICIENT[NB_CROP][NB_COEFICIENT] = 
{
	{ 1.270736317, 0.008436622,  0.00000000000,  0.000000000000,  0.000000000000000},	//Alfalfa Hay
	{ 0.042169500, 0.001508384,  0.00000489000, -0.000000008600,  0.000000000002490},	//Barley
	{ 1.012406972, 0.004093333,  0.00000000000,  0.000000000000,  0.000000000000000},	//Brome Hay
	{-0.013786236, 0.002504156,  0.00000080900, -0.000000003440,  0.000000000000949},	//Canary Seed
	{ 0.058456185, 0.001562771,  0.00000211692, -0.000000003120,  4.02098000000E-14},	//Canola
	{-0.036600000, 0.002530000, -0.00000218000,  0.000000000759, -0.000000000000292},	//Dry Bean
	{ 0.029786886, 0.003157012, -0.00000384000,  0.000000002910, -0.000000000001440},	//Dry peas
	{ 0.074200017, 0.001869647,  0.00000086600, -0.000000001700, -0.000000000000289},	//Flax
	{ 0.070868114, 0.001227545, -0.00000010700,  0.000000000304, -0.000000000000483},	//Fresh Corn (Sweet)
	{ 0.008566450, 0.002831325, -0.00000373000,  0.000000004550, -0.000000000003030},	//Fresh peas
	{ 0.007643393, 0.001791200, -0.00000253700,  0.000000003560, -0.000000000001790},	//Grain Corn
	{ 1.010846948, 0.004108323,  0.00000000000,  0.000000000000,  0.000000000000000},	//Grass Hay
	{ 0.050476113, 0.001064093,  0.00000472000, -0.000000006340,  0.000000000000892},	//Green Feed
	{-0.019248452, 0.002641005,  0.00000010500, -0.000000002230,  0.000000000000357},	//Hard Red Spring Weat
	{-0.018758692, 0.002617626,  0.00000023900, -0.000000002470,  0.000000000000427},	//Lentil
	{ 0.042169500, 0.001508384,  0.00000489000, -0.000000008690,  0.000000000002490},	//Malt Barley
	{ 0.993648105, 0.004248509,  0.00000000000,  0.000000000000,  0.000000000000000},	//milk vetch
	{-0.016999850, 0.002565810,  0.00000051900, -0.000000002960,  0.000000000000693},	//Monarda
	{ 0.063652214, 0.001506814,  0.00000229000, -0.000000003310,  0.000000000000117},	//Mustard
	{ 1.003023657, 0.004172678,  0.00000000000,  0.000000000000,  0.000000000000000},	//Native Pasture
	{ 0.079901221, 0.002003118,  0.00000175000, -0.000000003740,  0.000000000000476},	//Oat
	{ 0.081187870, 0.001973085,  0.00000188000, -0.000000003910,  0.000000000000525},	//Oat Silage
	{ 0.111115968, 0.004038038, -0.00000778000,  0.000000006830, -0.000000000002270},	//Ognon
	{ 0.056000000, 0.001770000, -0.00000190000,  0.000000001500, -0.000000000000540},	//Potato
	{-0.020333681, 0.002653702,  0.00000006300, -0.000000002180,  0.000000000000276},	//Rye
	{ 0.048130349, 0.002971681, -0.00000595000,  0.000000006740, -0.000000000002780},	//Safflower
	{ 0.046119780, 0.001466008,  0.00000501000, -0.000000008820,  0.000000000002530},	//Small Fruit
	{-0.020707159, 0.002658318,  0.00000004710, -0.000000002160,  0.000000000000267},	//Soft Weat
	{-0.012881686, 0.002426366, -0.00000274000,  0.000000002270, -0.000000000000877},	//Sugar Beets
	{ 0.025197777, 0.002945167, -0.00000577000,  0.000000005990, -0.000000000002170},	//Sunflower
	{ 1.010846948, 0.004108323,  0.00000000000,  0.000000000000,  0.000000000000000},	//Timothy Hay
	{-0.023511486, 0.002732458, -0.00000032000, -0.000000001570, -0.000000000000033},	//Tritical
	{ 0.819440290, 0.005861902,  0.00000000000,  0.000000000000,  0.000000000000000},	//Turn sod
	{-0.042728964, 0.004515424, -0.00000495000,  0.000000001980, -0.000000000001100}	//Winter weat
};


double CKropCoeficient::GetKc(size_t type, double x)
{
	ASSERT( type>=0 && type<NB_CROP);
		
	double a = KROP_COEFICIENT[type][A];
	double b = KROP_COEFICIENT[type][B];
	double c = KROP_COEFICIENT[type][C];
	double d = KROP_COEFICIENT[type][D];
	double e = KROP_COEFICIENT[type][E];
	double Kc = a + b*x + c*x*x + d*x*x*x + e*x*x*x*x;

	return max(0.1, Kc);
}

struct	CTest
{
	char* name;
	double F[5];
	int		test;
	double	D1;
	double	D2;
	int		DD;
	double P1;
	double P2;
};

const CTest KROP_COEFICIENT2[42] =
{
	{ "HARD RED SPRING WHEAT", -0.019248452, 0.002641005, 1.05E-07, -2.23E-09, 3.57E-13, 0, 1, 0.5, 44, 0.7, 0.3 },
	{ "OATS",			0.079901221, 0.002003118, 1.75E-06, -3.74E-09, 4.76E-13, 0, 1, 0.5, 55, 0.7, 0.3 },
	{ "BARLEY",			0.0421695, 0.001508384, 4.89E-06, -8.60E-09, 2.49E-12, 0, 1, 0.5, 36, 0.7, 0.3 },
	{ "FLAX",			0.074200017, 0.001869647, 8.66E-07, -1.70E-09, -2.89E-13, 0, 1, 0.5, 58, 0.7, 0.3 },
	{ "CANOLA",			0.08705, 0.00713, -1.96E-05, 2.31E-08, -9.63E-12, 0, 1, 0.5, 49, 0.7, 0.3 },
	{ "DRY PEAS",		0.029786886, 0.003157012, -0.00000384, 2.91E-09, -1.44E-12, 0, 0.8, 0.5, 41, 0.7, 0.3 },
	{ "SUGAR BEETS",	-0.151, 0.00447, -6.28E-06, 4.23E-09, -1.25E-12, 0, 1, 0.5, 99, 0.7, 0.3 },
	{ "POTATO",			0.056, 0.00177, -1.90E-06, 1.50E-09, -5.40E-13, 0, 0.8, 0.75, 88, 0.7, 0.3 },
	{ "GRAIN CORN",		0.007643393, 0.0017912, -0.000002537, 3.56E-09, -1.79E-12, 0, 1, 0.5, 68, 0.7, 0.3 },
	{ "FABA BEANS",		0.023049324, 0.001669346, -5.34E-07, 1.50E-09, -1.40E-12, 0, 0.8, 0.5, 64, 0.7, 0.3 },
	{ "BARLEY SILAGE UNDERSEED", 0.045178727, 0.001440247, 5.23E-06, -9.26E-09, 2.79E-12, 1, 1, 0.5, 36, 0.7, 0.3 },
	{ "BARLEY SILAGE",	0.044847195, 0.001440046, 5.25E-06, -9.33E-09, 2.84E-12, 0, 1, 0.5, 36, 0.7, 0.3 },
	{ "OATS SILAGE",	0.08118787, 0.001973085, 1.88E-06, -3.91E-09, 5.25E-13, 0, 1, 0.5, 55, 0.7, 0.3 },
	{ "CORN SILAGE",	0.006470394, 0.001813624, -0.00000262, 3.69E-09, -1.86E-12, 0, 1, 0.5, 68, 0.7, 0.3 },
	{ "ALFALFA HAY",	1.270736317, 0.008436622, 0, 0, 0, 1, 1.2, 0.5, 0, 0.6, 0.4 },
	{ "DRY BEANS",		-0.0366, 0.00253, -2.18E-06, 7.59E-10, -2.92E-13, 0, 0.9, 0.5, 68, 0.7, 0.3 },
	{ "GREEN FEED",		0.050476113, 0.001064093, 4.72E-06, -6.34E-09, 8.92E-13, 0, 1, 0.5, 36, 0.7, 0.3 },
	{ "GRASS HAY",		1.010846948, 0.004108323, 0, 0, 0, 1, 0.9, 0.5, 0, 0.65, 0.35 },
	{ "TIMOTHY HAY",	0.934, 0.012, 0, 0, 0, 1, 1.2, 0.5, 0, 0.65, 0.35 },
	{ "TRITICALE",		-0.023511486, 0.002732458, -3.20E-07, -1.57E-09, -3.30E-14, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "RYE",			-0.020333681, 0.002653702, 6.30E-08, -2.18E-09, 2.76E-13, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "SOFT WHEAT",		-0.020707159, 0.002658318, 4.71E-08, -2.16E-09, 2.67E-13, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "DURUM WHEAT",	-0.019139695, 0.002640079, 1.07E-07, -2.23E-09, 2.98E-13, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "WINTER WHEAT",	-0.042728964, 0.004515424, -4.95E-06, 1.98E-09, -1.10E-12, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "CPS WHEAT",		0.047946082, 0.002982562, -0.00000134, -5.84E-10, -4.15E-13, 0, 1, 0.5, 44, 0.7, 0.3 },
	{ "MUSTARD",		0.063652214, 0.001506814, 2.29E-06, -3.31E-09, 1.17E-13, 0, 1, 0.5, 49, 0.7, 0.3 },
	{ "LINOLA",			0.074200017, 0.001869647, 8.66E-07, -1.70E-09, -2.89E-13, 0, 1, 0.5, 58, 0.7, 0.3 },
	{ "ALFALFA SEED",	0.104519637, 0.003820155, -0.000006771, 4.83E-09, -1.25E-12, 0, 2.1, 0.25, 24, 0.6, 0.4 },
	{ "CANARY SEED",	-0.013786236, 0.002504156, 8.09E-07, -3.44E-09, 9.49E-13, 0, 1, 0.5, 40, 0.7, 0.3 },
	{ "CARROTS",		0.00161084, 0.001382806, -0.00000194, 2.22E-09, -9.93E-13, 0, 0.75, 0.7, 72, 0.7, 0.3 },
	{ "FRESH CORN(SWEET)", 0.070868114, 0.001227545, -1.07E-07, 3.04E-10, -4.83E-13, 0, 1, 0.5, 66, 0.7, 0.3 },
	{ "MINT",			0.047399792, 0.001379668, 0.0000056, -0.00000001, 3.28E-12, 0, 0.8, 0.6, 52, 0.7, 0.3 },
	{ "DILL",			-0.019515265, 0.002645237, 8.74E-08, -2.21E-09, 2.86E-13, 0, 1.2, 0.5, 52, 0.7, 0.3 },
	{ "GRASS SEED",		1.012406988, 0.004093333, 0, 0, 0, 1, 0.9, 0.5, 24, 0.7, 0.3 },
	{ "LENTILS",		-0.018758692, 0.002617626, 2.39E-07, -2.47E-09, 4.27E-13, 0, 1, 0.5, 42, 0.7, 0.3 },
	{ "ONIONS",			0.111115968, 0.004038038, -7.78E-06, 6.83E-09, -2.27E-12, 0, 0.4, 0.75, 56, 0.7, 0.3 },
	{ "FRESH PEAS",		0.00856645, 0.002831325, -3.73E-06, 4.55E-09, -3.03E-12, 0, 0.8, 0.6, 44, 0.7, 0.3 },
	{ "SEED POTATOES",	0.064834414, 0.001961444, -1.76E-06, 1.49E-09, -7.16E-13, 0, 0.8, 0.75, 88, 0.7, 0.3 },
	{ "SUNFLOWER",		0.025197777, 0.002945167, -5.77E-06, 5.99E-09, -2.17E-12, 0, 1, 0.5, 64, 0.7, 0.3 },
	{ "TURF SOD",		0.81944029, 0.005861902, 0, 0, 0, 1, 0.15, 0.5, 12, 0.7, 0.3 },
	{ "MISC.",			0.049469687, 0.001076361, 4.79E-06, -6.76E-09, 1.34E-12, 0, 0.3, 0.5, 40, 0.7, 0.3 },
	{ "MALT BARLEY",	0.0421695, 0.001508384, 4.89E-06, -8.69E-09, 2.49E-12, 0, 1, 0.5, 36, 0.7, 0.3 },
};
//Alfalfa Hay;Barley;Brome Hay;Canary Seed;Canola;Dry Bean;Dry Peas;Flax; Fresh Corn Sweet;Fresh Peas;Grain Corn; Grass Hay;Green Feed;Hard Red Spring weat;Lentil;Malt Barley;Milk Vetch;Monarda;Mustard;Native Pasture;Oat;Oat Silage;Onions;Potato;Rye;Safflower;Small Fruit;Soft Weat;Sugar Beets;Sunflower;Timothy Hay;Tritical;Turn Sod;Winter Weat

}//namespace WBSF 

