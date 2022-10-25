//**********************************************************************
// 25/10/2022	1.0		Rémi Saint-Amant	Creation
//**********************************************************************

#include "CCModel.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CCCModel::CreateObject);

	//Contructor
	CCCModel::CCCModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.6.0 (2022)";
	}

	CCCModel::~CCCModel()
	{}

	enum TAnnualStat{ O_ANNUAL_TMIN, O_ANNUAL_TMAX, O_ANNUAL_TAVG, O_ANNUAL_PREC, O_ANNUAL_GDD, O_ANNUAL_TSEA, O_ANNUAL_PSEA, O_ANNUAL_PWV, O_ANNUAL_PWM, O_ANNUAL_PDV, O_ANNUAL_PDM, O_ANNUAL_TRNG, O_ANNUAL_PET, O_ANNUAL_PETSEA, O_ANNUAL_WBAL, O_ANNUAL_WBALSEA, O_ANNUAL_WRAT, O_ANNUAL_CDD, O_ANNUAL_FSDAY, O_ANNUAL_FFDAY, O_ANNUAL_FPP, O_ANNUAL_FDD, O_ANNUAL_GSSDAY, O_ANNUAL_GSFDAY, O_ANNUAL_GSPP, O_ANNUAL_GSDD, NB_ANNUAL_STATS };
	enum TMonthlyStat{ O_MONTHLY_TMIN, O_MONTHLY_TMAX, O_MONTHLY_TAVG, O_MONTHLY_PREC, O_MONTHLY_GDD, O_MONTHLY_TSEA, O_MONTHLY_PSEA, O_MONTHLY_PET, O_MONTHLY_WBAL, O_MONTHLY_WRAT, O_MONTHLY_FDD, NB_MONTHLY_STATS };
	typedef CModelStatVectorTemplate<NB_ANNUAL_STATS> CAnnualStatVector;
	typedef CModelStatVectorTemplate<NB_MONTHLY_STATS> CMontlhyStatVector;


	ERMsg CCCModel::OnExecuteMonthly()
	{
		ERMsg msg;

		//CMontlhyStatVector stat(m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY)), 0);
		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY)), NB_MONTHLY_STATS, 0);

		//CTTransformation TT(m_weather.GetEntireTPeriod(), stat.GetTM());

		CModifiedHamonET HamonET;

		CModelStatVector ET;
		HamonET.Execute(m_weather, ET);
		ET.Transform(CTM::MONTHLY, SUM);
		//CModelStatVector monthlyET(ET, CTM::MONTHLY, SUM);

		CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, 5);
		CModelStatVector DD5;
		DD.Execute(m_weather, DD5);
		DD5.Transform(CTM::MONTHLY, SUM);
		//HamonET.Transform(TT, ET, monthlyET);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				size_t mm = y * 12 + m;
				
				m_output[mm][O_MONTHLY_TMIN] = m_weather[y][m][H_TMIN][MEAN];
				m_output[mm][O_MONTHLY_TMAX] = m_weather[y][m][H_TMAX][MEAN];
				m_output[mm][O_MONTHLY_TAVG] = m_weather[y][m][H_TAIR][MEAN];
				m_output[mm][O_MONTHLY_PREC] = m_weather[y][m][H_PRCP][SUM];
				m_output[mm][O_MONTHLY_GDD] = DD5[mm][0];
				m_output[mm][O_MONTHLY_TSEA] = GetTsea(m_weather[y][m]);
				m_output[mm][O_MONTHLY_PSEA] = GetPsea(m_weather[y][m]);
				m_output[mm][O_MONTHLY_PET] = ET[mm][0];
				m_output[mm][O_MONTHLY_WBAL] = m_output[mm][O_MONTHLY_PREC] - m_output[mm][O_MONTHLY_PET];
				m_output[mm][O_MONTHLY_WRAT] = m_output[mm][O_MONTHLY_PET] > 0 ? m_output[mm][O_MONTHLY_PREC] / m_output[mm][O_MONTHLY_PET] : 0;
				m_output[mm][O_MONTHLY_FDD] = GetCoolingDD(m_weather[y][m], 0);
			}
		}


		return msg;
	}

	ERMsg CCCModel::OnExecuteAnnual()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_ANNUAL_STATS, 0);

		CModifiedHamonET HamonET;
		CModelStatVector ET;
		HamonET.Execute(m_weather, ET);
		ET.Transform(CTM::MONTHLY, SUM);
		

		CDegreeDays CDD5(CDegreeDays::DAILY_AVERAGE, 5);
		CModelStatVector DD5;
		CDD5.Execute(m_weather, DD5);
		DD5.Transform(CTM::ANNUAL, SUM);

		CDegreeDays CDD0(CDegreeDays::DAILY_AVERAGE, 0);
		CModelStatVector DD0;
		CDD0.Execute(m_weather, DD0);

		CGrowingSeason GS;
		CFrostFreePeriod  FF;
		
		


		std::vector<double> Pwv;
		std::vector<size_t> Pwm;
		GetPwv(Pwv, Pwm);

		std::vector<double> Pdv;
		std::vector<size_t> Pdm;
		GetPdv(Pdv, Pdm);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::MONTHLY);
			CTPeriod GSp = GS.GetPeriod(m_weather[y]);//definition have probably change since the first version
			CTPeriod FFp = FF.GetPeriod(m_weather[y]);
			

			CStatistic ETStat = ET.GetStat(0, p);
			m_output[y][O_ANNUAL_TMIN] = m_weather[y][H_TMIN][MEAN];
			m_output[y][O_ANNUAL_TMAX] = m_weather[y][H_TMAX][MEAN];
			m_output[y][O_ANNUAL_TAVG] = m_weather[y][H_TAIR][MEAN];
			m_output[y][O_ANNUAL_PREC] = m_weather[y][H_PRCP][SUM];
			m_output[y][O_ANNUAL_GDD] = DD5[y][0];
			m_output[y][O_ANNUAL_TSEA] = GetTsea(m_weather[y]);
			m_output[y][O_ANNUAL_PSEA] = GetPsea(m_weather[y]);
			m_output[y][O_ANNUAL_PWV] = Pwv[y];
			m_output[y][O_ANNUAL_PWM] = Pwm[y] + 1;
			m_output[y][O_ANNUAL_PDV] = Pdv[y];
			m_output[y][O_ANNUAL_PDM] = Pdm[y] + 1;
			m_output[y][O_ANNUAL_TRNG] = GetMonthlyDifference(m_weather[y]);
			m_output[y][O_ANNUAL_PET] = ETStat[SUM];
			m_output[y][O_ANNUAL_PETSEA] = ETStat[COEF_VAR];// GetPETsea(ET);
			m_output[y][O_ANNUAL_WBAL] = m_output[y][O_ANNUAL_PREC] - m_output[y][O_ANNUAL_PET];
			m_output[y][O_ANNUAL_WBALSEA] = GetWBALsea(m_weather[y], ET);
			m_output[y][O_ANNUAL_WRAT] = m_output[y][O_ANNUAL_PET] > 0 ? m_output[y][O_ANNUAL_PREC] / m_output[y][O_ANNUAL_PET] : 0;
			m_output[y][O_ANNUAL_CDD] = GetCoolingDD(m_weather[y], 0);

			//frost free period
			m_output[y][O_ANNUAL_FSDAY] = FFp.Begin().GetJDay() + 1;
			m_output[y][O_ANNUAL_FFDAY] = FFp.End().GetJDay() + 1;
			m_output[y][O_ANNUAL_FPP] = FFp.GetLength();
			m_output[y][O_ANNUAL_FDD] = DD0.GetStat(0, FFp)[SUM]; //m_weather[y].GetDD(0, Fp);

			
			//growing season period
			m_output[y][O_ANNUAL_GSSDAY] = GSp.Begin().GetJDay() + 1;
			m_output[y][O_ANNUAL_GSFDAY] = GSp.End().GetJDay() + 1;
			m_output[y][O_ANNUAL_GSPP] = GSp.GetLength();
			m_output[y][O_ANNUAL_GSDD] = DD0.GetStat(0, GSp)[SUM]; //m_weather[y].GetDD(0, GSp);
		}

		


		return msg;
	}

	//daily coeficeint of variation on temperature
	double CCCModel::GetTsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			stat += weather[d][H_TAIR][MEAN];
		}

		return stat[COEF_VAR];
	}

	double CCCModel::GetPsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			stat += weather[d][H_PRCP][SUM];
		}

		return stat[COEF_VAR];
	}


	void CCCModel::GetPwv(std::vector<double>& Pwv, std::vector<size_t>& Pwm)
	{
		//int nbOcc[12] = {0};
		Pwv.resize(m_weather.GetNbYears());
		Pwm.resize(m_weather.GetNbYears());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			//find the higest month
			double prcpMax = -1;
			size_t index = NOT_INIT;
			for (size_t m = 0; m < 12; m++)
			{
				double prcp = m_weather[y][m][H_PRCP][SUM];
				if (prcp > prcpMax)
				{
					prcpMax = prcp;
					index = m;
				}
			}

			Pwv[y] = prcpMax;
			Pwm[y] = index;
		}
	}

	void CCCModel::GetPdv(std::vector<double>& Pdv, std::vector<size_t>& Pdm)
	{
		Pdv.resize(m_weather.GetNbYears());
		Pdm.resize(m_weather.GetNbYears());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			//find the higest month
			double prcpMin = 999999999999;
			size_t index = NOT_INIT;
			for (size_t m = 0; m < 12; m++)
			{
				double prcp = m_weather[y][m][H_PRCP][SUM];
				if (prcp < prcpMin)
				{
					prcpMin = prcp;
					index = m;
				}
			}

			Pdv[y] = prcpMin;
			Pdm[y] = index;
		}
	}

	double CCCModel::GetMonthlyDifference(const CWeatherYear& weather)
	{
		CStatistic statTmin;
		CStatistic statTmax;
		for (size_t m = 0; m < 12; m++)
		{
			statTmin += weather[m][H_TMIN][MEAN];
			statTmax += weather[m][H_TMAX][MEAN];
		}

		return statTmax[HIGHEST] - statTmin[LOWEST];
	}

	double CCCModel::GetTsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += weather[m][H_TAIR][MEAN];
		}

		return stat[COEF_VAR];
	}

	double CCCModel::GetPsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += weather[m][H_PRCP][SUM];
		}

		return stat[COEF_VAR];
	}

/*
	double CCCModel::GetPETsea(const CModelStatVector& HamonPET)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
			stat += HamonPET[m][0];

		return stat[COEF_VAR];
	}*/

	double CCCModel::GetWBALsea(const CWeatherYear& weather, const CModelStatVector& HamonPET)
	{
		int year = weather.GetTRef().GetYear();
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += (weather[m][H_PRCP][SUM] - HamonPET[CTRef(year,m)][0]);
		}
			

		return stat[COEF_VAR];
	}

	double CCCModel::GetCoolingDD(const CWeatherYear& weather, double threshold)
	{
		double FDD = 0;
		for (size_t jd = 0; jd < weather.GetNbDays(); jd++)
		{
			if (weather.GetDay(jd)[H_TAIR][MEAN] < 0)
				FDD += -weather.GetDay(jd)[H_TAIR][MEAN];
		}

		return FDD;
	}

	double CCCModel::GetCoolingDD(const CWeatherMonth& weather, double threshold)
	{
		double FDD = 0;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			if (weather[d][H_TAIR][MEAN] < 0)
				FDD += -weather[d][H_TAIR][MEAN];
		}

		return FDD;
	}

}