//************** M O D I F I C A T I O N S   L O G ********************
//10/10/2005 Rémi Saint-Amant	Creation
//15/11/2007 Rémi Saint-Amant	Compile with visual C++ 8
//22/07/2011 Rémi Saint-Amant	New version for BioSIM 10 and VC 2010
//10/01/2014 Rémi Saint-Amant	New version for BioSIM 10.5
//*********************************************************************
#include "CMIModel.h"
#include "EntryPoint.h"
#include "Weather.h"

using namespace CFL;

//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CCMIModel::CreateObject );


CCMIModel::CCMIModel()
{
	NB_INPUT_PARAMETER = 0;
	VERSION = "2.1 (2014)";
}

	
ERMsg CCMIModel::OnExecuteAnnual()
{
	ERMsg msg;

	//Get elevation
	double elev = m_info.m_loc.m_elev;
	
	//Create output vector
	CAnnualOutput output(m_weather.GetNbYear()-1, CTRef(m_weather[1].GetYear()) );

	//compute variable for all year except the first one
	for(int y=1; y<m_weather.GetNbYear(); y++)
	{
		short year = m_weather[y].GetYear();
		CTPeriod p( year-1, AUGUST, FIRST_DAY, year, JULY, LAST_DAY);

		float gddcum = m_weather.GetDD(5, CTPeriod(year, JANUARY, FIRST_DAY, year, JULY, LAST_DAY));
		float gddwyr = m_weather.GetDD(5, p);

		float pptSummer = m_weather[y].GetStat(STAT_PRCP, SUM, CTPeriod(year, MARCH, FIRST_DAY, year, JUNE, LAST_DAY) )/10;//in cm
		
		//conversion from mm to cm
		float Pwyr = m_weather.GetStat(STAT_PRCP, SUM, p)/10;
		float PETwyr = GetSPMPET(m_weather, p, elev )/10;

		float tmaxwyr = m_weather.GetStat(STAT_TMAX, MEAN, p);
		float tminwyr = m_weather.GetStat(STAT_TMIN, MEAN, p);
		
		float CMIwyr = Pwyr - PETwyr;

		output[y-1][O_GDD_CUM] = gddcum;
		output[y-1][O_GDD_WYR] = gddwyr;
		output[y-1][O_CMI_WYR] = CMIwyr;
		output[y-1][O_PPT_WYR] =  Pwyr; 
		output[y-1][O_PET_WYR] = PETwyr; 
		output[y-1][O_TMAX_WYR] = tmaxwyr; 
		output[y-1][O_TMIN_WYR] =  tminwyr;
		output[y-1][O_PPT_SUMMER] = pptSummer;

	}

	//Save outputs
	SetOutput(output);

	return msg;
}

//Compute monthly value
ERMsg CCMIModel::OnExecuteMonthly()
{
	ERMsg msg;

	//Get elevation
	double elev = m_info.m_loc.m_elev;

	//Create output vector
	
	CMonthlyOutput output(m_weather.GetNbYear()*12, CTRef(m_weather.GetFirstYear(), FIRST_MONTH) );
	
	//compute CMI for each months of all years
	for(int y=0; y<m_weather.GetNbYear(); y++)
	{
		for(int m=0; m<12; m++)
		{
			float TMaxMean  = m_weather[y][m].GetStat(STAT_TMAX, MEAN);
			float TMinMean  = m_weather[y][m].GetStat(STAT_TMIN, MEAN);
			float TMeanMean  = m_weather[y][m].GetStat(STAT_T_MN, MEAN);
			
			float pptSum = m_weather[y][m].GetStat(STAT_PRCP, SUM)/10;//in cm
			float PETSum = GetSPMPET(m_weather[y][m].GetStat(), elev)/10;//in cm
			float CMI = Max(0, pptSum - PETSum);
			
			output[y*12+m][O_TMAX_MEAN] = TMaxMean;
			output[y*12+m][O_TMIN_MEAN] = TMinMean;
			output[y*12+m][O_PPT_SUM] = pptSum;
			output[y*12+m][O_PET_SUM] = PETSum;
			output[y*12+m][O_CMI] = CMI;
		}
   }

	//Save outputs
	SetOutput(output);

	return msg;
}


//Calculate Simplified Penman-Monteith PET (monthly)
float CCMIModel::GetSPMPET(const CWeatherStatistic& stat, short elev )
{
	//input monthly tmax, tmin, prec and calculate tmean
	float TMax = stat[STAT_TMAX][MEAN];
	float TMin = stat[STAT_TMIN][MEAN];
	float TMean = stat[STAT_T_MN][MEAN];

	//First calculate SVP for monthly tmax tmin and tdew (assumed = tmin - 2.5)
	double SVPtmax = .61078 * exp(17.269 * TMax/ (237.3 + TMax));
	double SVPtmin = .61078 * exp(17.269 * TMin / (237.3 + TMin));
	double tdew = TMin - 2.5;
	double SVPtdew = .61078 * exp(17.269 * tdew / (237.3 + tdew));
	double VPD = .5 * (SVPtmax + SVPtmin) - SVPtdew;

//Now calculate PET from Hogg 1997
	double PET = 0;
	if( TMean > 10)
		PET = 93 * VPD * exp(double(elev / 9300.0));
	else if(TMean > -5)
		PET = (6.2 * TMean + 31) * VPD * exp(elev / 9300.0);
	else
		PET = 0;

	return PET;
}


//Calculate Simplified Penman-Monteith PET (annual)
float CCMIModel::GetSPMPET(CWeather& weather, CTPeriod& p, short elev )
{

	float PETwyr = 0;

	for(int y=0; y<weather.GetNbYear(); y++)
	{
		short year = weather.GetFirstYear() + y;
		
		for(int m=0; m<12; m++)
		{
			if( p.IsInside( CTRef( year, m, 15) ) )//is this month is used
			{
				float PET = GetSPMPET(weather[y][m].GetStat(), elev);
				PETwyr += PET;
			}
		}
	}

	return PETwyr;
}
