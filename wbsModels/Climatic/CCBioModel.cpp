//**********************************************************************
// 11/09/2018	2.5.1	Rémi Saint-Amant    Compile with VS 2017
// 20/09/2016	2.5.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	2.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 03/10/2013	2.3		Rémi Saint-Amant	New compilation (x64)
// 07/02/2012   2.2		Rémi Saint-Amant	Frost free end period is shifted of one day (bug)
// 15/11/2010	2.1		Rémi Saint-Amant	Creation
//**********************************************************************

#include "Basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"
#include "CCBioModel.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CCCBioModel::CreateObject);

	//Contructor
	CCCBioModel::CCCBioModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.5.1 (2018)";
	}

	CCCBioModel::~CCCBioModel()
	{}

	enum TAnnualStat{ O_ANNUAL_TMIN, O_ANNUAL_TMAX, O_ANNUAL_TAVG, O_ANNUAL_PREC, O_ANNUAL_GDD, O_ANNUAL_TSEA, O_ANNUAL_PSEA, O_ANNUAL_PWV, O_ANNUAL_PWM, O_ANNUAL_PDV, O_ANNUAL_PDM, O_ANNUAL_TRNG, O_ANNUAL_PET, O_ANNUAL_PETSEA, O_ANNUAL_WBAL, O_ANNUAL_WBALSEA, O_ANNUAL_WRAT, O_ANNUAL_CDD, O_ANNUAL_FSDAY, O_ANNUAL_FFDAY, O_ANNUAL_FPP, O_ANNUAL_FDD, O_ANNUAL_GSSDAY, O_ANNUAL_GSFDAY, O_ANNUAL_GSPP, O_ANNUAL_GSDD, NB_ANNUAL_STATS };
	enum TMonthlyStat{ O_MONTHLY_TMIN, O_MONTHLY_TMAX, O_MONTHLY_TAVG, O_MONTHLY_PREC, O_MONTHLY_GDD, O_MONTHLY_TSEA, O_MONTHLY_PSEA, O_MONTHLY_PET, O_MONTHLY_WBAL, O_MONTHLY_WRAT, O_MONTHLY_FDD, NB_MONTHLY_STATS };
	typedef CModelStatVectorTemplate<NB_ANNUAL_STATS> CAnnualStatVector;
	typedef CModelStatVectorTemplate<NB_MONTHLY_STATS> CMontlhyStatVector;


	ERMsg CCCBioModel::OnExecuteMonthly()
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
				
				m_output[mm][O_MONTHLY_TMIN] = m_weather[y][m][H_TMIN2][MEAN];
				m_output[mm][O_MONTHLY_TMAX] = m_weather[y][m][H_TMAX2][MEAN];
				m_output[mm][O_MONTHLY_TAVG] = m_weather[y][m][H_TAIR2][MEAN];
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

	ERMsg CCCBioModel::OnExecuteAnnual()
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
			CTPeriod GSp = GS.GetGrowingSeason(m_weather[y]);
			CTPeriod Fp = GS.GetFrostFreePeriod(m_weather[y]);
			//CTPeriod Fp = m_weather[y].GetFrostFreePeriod();
			//CTPeriod GSp = m_weather[y].GetGrowingSeason();

			CStatistic ETStat = ET.GetStat(0, p);
			m_output[y][O_ANNUAL_TMIN] = m_weather[y][H_TMIN2][MEAN];
			m_output[y][O_ANNUAL_TMAX] = m_weather[y][H_TMAX2][MEAN];
			m_output[y][O_ANNUAL_TAVG] = m_weather[y][H_TAIR2][MEAN];
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
			m_output[y][O_ANNUAL_FSDAY] = Fp.Begin().GetJDay() + 1;
			m_output[y][O_ANNUAL_FFDAY] = Fp.End().GetJDay() + 1;
			m_output[y][O_ANNUAL_FPP] = Fp.GetLength();
			m_output[y][O_ANNUAL_FDD] = DD0.GetStat(0, Fp)[SUM]; //m_weather[y].GetDD(0, Fp);

			
			//growing season period
			m_output[y][O_ANNUAL_GSSDAY] = GSp.Begin().GetJDay() + 1;
			m_output[y][O_ANNUAL_GSFDAY] = GSp.End().GetJDay() + 1;
			m_output[y][O_ANNUAL_GSPP] = GSp.GetLength();
			m_output[y][O_ANNUAL_GSDD] = DD0.GetStat(0, GSp)[SUM]; //m_weather[y].GetDD(0, GSp);
		}

		


		return msg;
	}

	//daily coeficeint of variation on temperature
	double CCCBioModel::GetTsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			stat += weather[d][H_TAIR2][MEAN];
		}

		return stat[COEF_VAR];
	}

	double CCCBioModel::GetPsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			stat += weather[d][H_PRCP][SUM];
		}

		return stat[COEF_VAR];
	}


	void CCCBioModel::GetPwv(std::vector<double>& Pwv, std::vector<size_t>& Pwm)
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

	void CCCBioModel::GetPdv(std::vector<double>& Pdv, std::vector<size_t>& Pdm)
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

	double CCCBioModel::GetMonthlyDifference(const CWeatherYear& weather)
	{
		CStatistic statTmin;
		CStatistic statTmax;
		for (size_t m = 0; m < 12; m++)
		{
			statTmin += weather[m][H_TMIN2][MEAN];
			statTmax += weather[m][H_TMAX2][MEAN];
		}

		return statTmax[HIGHEST] - statTmin[LOWEST];
	}

	double CCCBioModel::GetTsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += weather[m][H_TAIR2][MEAN];
		}

		return stat[COEF_VAR];
	}

	double CCCBioModel::GetPsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += weather[m][H_PRCP][SUM];
		}

		return stat[COEF_VAR];
	}

/*
	double CCCBioModel::GetPETsea(const CModelStatVector& HamonPET)
	{
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
			stat += HamonPET[m][0];

		return stat[COEF_VAR];
	}*/

	double CCCBioModel::GetWBALsea(const CWeatherYear& weather, const CModelStatVector& HamonPET)
	{
		int year = weather.GetTRef().GetYear();
		CStatistic stat;
		for (size_t m = 0; m < 12; m++)
		{
			stat += (weather[m][H_PRCP][SUM] - HamonPET[CTRef(year,m)][0]);
		}
			

		return stat[COEF_VAR];
	}

	double CCCBioModel::GetCoolingDD(const CWeatherYear& weather, double threshold)
	{
		double FDD = 0;
		for (size_t jd = 0; jd < weather.GetNbDays(); jd++)
		{
			if (weather.GetDay(jd)[H_TAIR2][MEAN] < 0)
				FDD += -weather.GetDay(jd)[H_TAIR2][MEAN];
		}

		return FDD;
	}

	double CCCBioModel::GetCoolingDD(const CWeatherMonth& weather, double threshold)
	{
		double FDD = 0;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			if (weather[d][H_TAIR2][MEAN] < 0)
				FDD += -weather[d][H_TAIR2][MEAN];
		}

		return FDD;
	}

}