//**********************************************************************
// 20/09/2016	1.5.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	1.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 14/02/2013			Rémi Saint-Amant	Creation
// This model is base on :
//	A.J. Fisher, L. Smith & D.M. Woods (2011): Climatic analysis to determine
//	where to collect and release Puccinia jaceae var. solstitialis for biological control of yellow
//	starthistle, Biocontrol Science and Technology, 21:3, 333-351
//**********************************************************************


#include "ModelBase/EntryPoint.h"
#include "Basic/Evapotranspiration.h"
//#include "StdFile.h"
#include "DewPeriod.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CDewPeriodModel::CreateObject);

	//Contructor
	CDewPeriodModel::CDewPeriodModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.5.0 (2016)";
	}


	CDewPeriodModel::~CDewPeriodModel()
	{}


	double CDewPeriodModel::GetTDew(double Tmax, double RH15)
	{
		static const double a = 17.27;
		static const double b = 237.7;
		const double c = (a*Tmax) / (b + Tmax) + log(min(100.0, max(1.0, RH15)) / 100);
		return (b*c) / (a - c);

		//return (237.7*(17.27*Tmax/(237.7+Tmax)+log(RH15/100)))/(17.27-(17.27*Tmax/(237.7+Tmax)+log(RH15/100)));
	}

	//Tmin : monthly mean Tmin
	//Tmax : monthly mean Tmax
	//Tdew15: Dew point computed from relative humidity at 15:00 h
	double CDewPeriodModel::GetDewPeriod(double Tmin, double Tmax, double Tdew15)
	{
		double DP = 0;

		//if(Tdew15>Tmax)
		//	WD=24;
		//else if(Tdew15>Tmin)
		//	WD=(asin(2*(Tdew15-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12;
		//else 
		//	WD=0;



		if (Tdew15 < Tmin)
			DP = 0;
		else if (Tdew15 > Tmax)
			DP = 24;
		else DP = (asin(2 * (Tdew15 - Tmin) / (Tmax - Tmin) - 1) * 2 / PI + 1) * 12;



		return DP;
	}

	/*double CDewPeriodModel::GetRH15(const CMonth& hourlyData)
	{
	CStatistic stat;
	for(size_t d=0; d<hourlyData.size(); d++)
	{
	stat+=hourlyData[d][15][HOURLY_DATA::H_RELH];
	}

	return stat[MEAN];
	}*/

	double CDewPeriodModel::GetHourlyNormal(size_t m, size_t h, size_t v, const CWeatherYears& hourlyData)
	{
		CStatistic stat;

		for (size_t y = 0; y < hourlyData.size(); y++)
		{
			int year = hourlyData[y].GetTRef().GetYear();
			for (size_t d = 0; d < hourlyData[y][m].size(); d++)
			{
				CTRef h(year, m, d, h);
				const CHourlyData& hData = (const CHourlyData&)hourlyData.Get(h);
				stat += hData[v];
			}
		}

		return stat[MEAN];
	}


	ERMsg CDewPeriodModel::OnExecuteMonthly()
	{
		ERMsg msg;

		/*CMonthlyStatVector stat(m_weather.GetNbYear()*12, CTRef(m_weather[0].GetYear(), CFL::FIRST_MONTH));



		CWeather tmp (m_weather);
		tmp.AjusteMeanForHourlyRequest(m_info.m_loc);

		CYearsVector hourlyData;
		tmp.GetHourlyVar(hourlyData, m_info.m_loc);


		for(int y=0; y<m_weather.GetNbYear(); y++)
		{
		short year = m_weather[y].GetYear();
		for(int m=0; m<12; m++)
		{
		double Tmin = m_weather[y][m].GetStat(TMIN, MEAN);
		double Tmax = m_weather[y][m].GetStat(TMAX, MEAN);
		double RH15 = GetRH15(hourlyData[y][m]);

		double WD=GetWetnessDuration(Tmin, Tmax, RH15);

		stat[y*12+m][MONTHLY_DEW_PERIOD] = WD;
		HxGridTestConnection();

		}
		}*/

		CMonthlyStatVector stat(12, CTRef(CTRef::MONTHLY, 0, 0, 0, CTRef::OVERALL_YEARS));


		//	3.9	11.1	97	89	76
		//6.1	14.4	71	87	66
		//7.8	17.8	71	84	58
		//8.9	20.6	38	82	50
		//11.1	24.4	20	81	46
		//13.3	28.9	3	79	38
		//14.4	32.2	0	78	34
		//13.9	31.7	0	79	36
		//13.3	28.9	8	77	37
		//10	23.9	20	79	44
		//6.7	17.8	48	83	57
		//3.9	12.2	97	89	76

		//double test1 = GetTDew(3.9, 11.1, 76);
		//double test2 = GetDewPeriod(3.9, 11.1, test1);
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		//CWeatherStation tmp(m_weather);

		//tmp.AjusteMeanForHourlyRequest(m_info.m_loc);

		//CYearsVector hourlyData;
		//tmp.GetHourlyVar(hourlyData, m_info.m_loc);


		for (size_t m = 0; m < 12; m++)
		{
			CTPeriod p(CTRef(YEAR_NOT_INIT, m, FIRST_DAY, FIRST_HOUR), CTRef(YEAR_NOT_INIT, m, LAST_DAY, LAST_HOUR), CTPeriod::YEAR_BY_YEAR);
			double Tmin = m_weather.GetStat(H_TMIN, p)[MEAN];
			double Tmax = m_weather.GetStat(H_TMAX, p)[MEAN];

			double RH15 = GetHourlyNormal(m, 15, H_RELH, m_weather);
			double Tdew15FromRH15 = GetTDew(Tmax, RH15);
			double DPFromRH15 = GetDewPeriod(Tmin, Tmax, Tdew15FromRH15);


			stat[m][O_TMIN] = Tmin;
			stat[m][O_TMAX] = Tmax;
			stat[m][O_RH15] = RH15;
			stat[m][O_TDEW15_FROM_RH15] = Tdew15FromRH15;
			stat[m][O_DEW_PERIOD_FROM_RH15] = DPFromRH15;

			HxGridTestConnection();
		}


		SetOutput(stat);


		return msg;
	}

}