//************** M O D I F I C A T I O N S   L O G ********************
//06/09/2016 Rémi Saint-Amant	Integration with WBSF
//10/01/2014 Rémi Saint-Amant	New version for BioSIM 10.5
//22/07/2011 Rémi Saint-Amant	New version for BioSIM 10 and VC 2010
//15/11/2007 Rémi Saint-Amant	Compile with visual C++ 8
//10/10/2005 Rémi Saint-Amant	Creation



//*********************************************************************
#include "CMIModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/WeatherStation.h"
#include "Basic/DegreeDays.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CCMIModel::CreateObject);


	CCMIModel::CCMIModel()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.1.1 (2016)";
	}


	ERMsg CCMIModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//Compute DD5
		CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, 5);
		CModelStatVector DD5;
		DD.Execute(m_weather, DD5);

		

		//Create output vector
		m_output.Init(m_weather.GetNbYears() - 1, CTRef(m_weather[1].GetTRef().GetYear()), NB_A_OUTPUT);

		//compute variable for all year except the first one
		for (size_t y = 1; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);
			CTPeriod p1(year - 1, AUGUST, FIRST_DAY, year, JULY, LAST_DAY);
			CTPeriod p2(year, JANUARY, FIRST_DAY, year, JULY, LAST_DAY);

			double gddwyr = DD5.GetStat(CDegreeDays::S_DD, p1)[SUM];
			double gddcum = DD5.GetStat(CDegreeDays::S_DD, p2)[SUM];
			//float gddcum = m_weather.GetDD(5, CTPeriod(year, JANUARY, FIRST_DAY, year, JULY, LAST_DAY));
			//float gddwyr = m_weather.GetDD(5, p);
			

			CTPeriod p3(year, MARCH, FIRST_DAY, year, JUNE, LAST_DAY);
			double pptSummer = m_weather[y].GetStat(H_PRCP, p3)[SUM] / 10;//in cm

			//conversion from mm to cm
			double Pwyr = m_weather.GetStat(H_PRCP, p1)[SUM] / 10;
			double PETwyr = GetSPMPET(m_weather, p1) / 10;

			double tmaxwyr = m_weather.GetStat(H_TMAX, p1)[MEAN];
			double tminwyr = m_weather.GetStat(H_TMIN, p1)[MEAN];

			double CMIwyr = Pwyr - PETwyr;

			m_output[y - 1][O_GDD_CUM] = gddcum;
			m_output[y - 1][O_GDD_WYR] = gddwyr;
			m_output[y - 1][O_CMI_WYR] = CMIwyr;
			m_output[y - 1][O_PPT_WYR] = Pwyr;
			m_output[y - 1][O_PET_WYR] = PETwyr;
			m_output[y - 1][O_TMAX_WYR] = tmaxwyr;
			m_output[y - 1][O_TMIN_WYR] = tminwyr;
			m_output[y - 1][O_PPT_SUMMER] = pptSummer;

		}

	

		return msg;
	}

	//Compute monthly value
	ERMsg CCMIModel::OnExecuteMonthly()
	{
		ERMsg msg;

		
		//Create output vector
		m_output.Init(m_weather.GetNbYears() * 12, CTRef(m_weather.GetFirstYear(), FIRST_MONTH), NB_M_OUTPUT);

		//compute CMI for each months of all years
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				double TMaxMean = m_weather[y][m].GetStat(H_TMAX)[MEAN];
				double TMinMean = m_weather[y][m].GetStat(H_TMIN)[MEAN];
				double TMeanMean = m_weather[y][m].GetStat(H_TAIR)[MEAN];

				double pptSum = m_weather[y][m].GetStat(H_PRCP)[SUM] / 10;//in cm
				double PETSum = GetSPMPET(m_weather[y][m]) / 10;//in cm
				double CMI = max(0.0, pptSum - PETSum);

				m_output[y * 12 + m][O_TMAX_MEAN] = TMaxMean;
				m_output[y * 12 + m][O_TMIN_MEAN] = TMinMean;
				m_output[y * 12 + m][O_PPT_SUM] = pptSum;
				m_output[y * 12 + m][O_PET_SUM] = PETSum;
				m_output[y * 12 + m][O_CMI] = CMI;
			}
		}

		

		return msg;
	}


	//Calculate Simplified Penman-Monteith PET (monthly)
	double CCMIModel::GetSPMPET(const CWeatherMonth& weather)
	{
		double elev = weather.GetLocation().m_elev;
		ASSERT(elev > -999);

		//input monthly tmax, tmin, prec and calculate tmean
		double TMax = weather[H_TMAX][MEAN];
		double TMin = weather[H_TMIN][MEAN];
		double TMean = weather[H_TAIR][MEAN];

		//First calculate SVP for monthly tmax tmin and tdew (assumed = tmin - 2.5)
		double SVPtmax = .61078 * exp(17.269 * TMax / (237.3 + TMax));
		double SVPtmin = .61078 * exp(17.269 * TMin / (237.3 + TMin));
		double tdew = TMin - 2.5;
		double SVPtdew = .61078 * exp(17.269 * tdew / (237.3 + tdew));
		double VPD = .5 * (SVPtmax + SVPtmin) - SVPtdew;

		//Now calculate PET from Hogg 1997
		double PET = 0;
		if (TMean > 10)
			PET = 93 * VPD * exp(double(elev / 9300.0));
		else if (TMean > -5)
			PET = (6.2 * TMean + 31) * VPD * exp(elev / 9300.0);
		else
			PET = 0;

		return PET;
	}


	//Calculate Simplified Penman-Monteith PET (annual)
	double CCMIModel::GetSPMPET(const CWeatherStation& weather, CTPeriod& p)
	{
		
		double PETwyr = 0;

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			int year = weather.GetFirstYear() + int(y);

			for (size_t m = 0; m < 12; m++)
			{
				if (p.IsInside(CTRef(year, m, 15)))//is this month is used
				{
					double PET = GetSPMPET(weather[y][m]);
					PETwyr += PET;
				}
			}
		}

		return PETwyr;
	}

}