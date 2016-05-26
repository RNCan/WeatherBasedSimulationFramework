//**********************************************************************
// 21/01/2016	2.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 03/10/2013	2.3		Rémi Saint-Amant	New compilation (x64)
// 07/02/2012   2.2		Rémi Saint-Amant	Frost free end period is shifted of one day (bug)
// 15/11/2010	2.1		Rémi Saint-Amant	Creation
//**********************************************************************

#include "Basic/Evapotranspiration.h"
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
		VERSION = "2.3.0 (2015)";
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

		CMontlhyStatVector stat(m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY)), 0);
		CTTransformation TT(m_weather.GetEntireTPeriod(), stat.GetTM());

		CModifiedHamonET HamonET;

		CModelStatVector ET;
		HamonET.Execute(m_weather, ET);
		CModelStatVector monthlyET;
		HamonET.Transform(TT, ET, monthlyET);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				size_t yy = y * 12 + m;

				stat[yy][O_MONTHLY_TMIN] = m_weather[y][m][H_TAIR][MEAN] - m_weather[y][m][H_TRNG][MEAN] / 2;
				stat[yy][O_MONTHLY_TMAX] = m_weather[y][m][H_TAIR][MEAN] + m_weather[y][m][H_TRNG][MEAN] / 2;
				stat[yy][O_MONTHLY_TAVG] = m_weather[y][m][H_TAIR][MEAN];
				stat[yy][O_MONTHLY_PREC] = m_weather[y][m][H_PRCP][SUM];
				stat[yy][O_MONTHLY_GDD] = m_weather[y][m].GetDD(5);
				stat[yy][O_MONTHLY_TSEA] = GetTsea(m_weather[y][m]);
				stat[yy][O_MONTHLY_PSEA] = GetPsea(m_weather[y][m]);
				stat[yy][O_MONTHLY_PET] = HamonET.Get(m);
				stat[yy][O_MONTHLY_WBAL] = stat[yy][O_MONTHLY_PREC] - stat[yy][O_MONTHLY_PET];
				stat[yy][O_MONTHLY_WRAT] = stat[yy][O_MONTHLY_PET] > 0 ? stat[yy][O_MONTHLY_PREC] / stat[yy][O_MONTHLY_PET] : 0;
				stat[yy][O_MONTHLY_FDD] = GetCoolingDD(m_weather[y][m], 0);
			}
		}

		SetOutput(stat);


		return msg;
	}

	ERMsg CCCBioModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CAnnualStatVector stat(m_weather.GetNbYear(), CTRef(m_weather[0].GetYear()));

		std::vector<double> Pwv;
		std::vector<int> Pwm;
		GetPwv(Pwv, Pwm);

		std::vector<double> Pdv;
		std::vector<int> Pdm;
		GetPdv(Pdv, Pdm);

		for (int y = 0; y < m_weather.GetNbYear(); y++)
		{
			CHamonPET HamonPET(m_weather[y], m_info.m_loc.GetLat());

			CTPeriod Fp = m_weather[y].GetFrostFreePeriod();
			CTPeriod GSp = m_weather[y].GetGrowingSeason();


			stat[y][O_ANNUAL_TMIN] = m_weather[y].GetStat(STAT_TMIN, MEAN);
			stat[y][O_ANNUAL_TMAX] = m_weather[y].GetStat(STAT_TMAX, MEAN);
			stat[y][O_ANNUAL_TAVG] = m_weather[y].GetStat(STAT_T_MN, MEAN);
			stat[y][O_ANNUAL_PREC] = m_weather[y].GetStat(STAT_PRCP, SUM);
			stat[y][O_ANNUAL_GDD] = m_weather[y].GetDD(5);
			stat[y][O_ANNUAL_TSEA] = GetTsea(m_weather[y]);
			stat[y][O_ANNUAL_PSEA] = GetPsea(m_weather[y]);
			stat[y][O_ANNUAL_PWV] = Pwv[y];
			stat[y][O_ANNUAL_PWM] = Pwm[y] + 1;
			stat[y][O_ANNUAL_PDV] = Pdv[y];
			stat[y][O_ANNUAL_PDM] = Pdm[y] + 1;
			stat[y][O_ANNUAL_TRNG] = GetMonthlyDifference(m_weather[y]);
			stat[y][O_ANNUAL_PET] = HamonPET.Get();
			stat[y][O_ANNUAL_PETSEA] = GetPETsea(HamonPET);
			stat[y][O_ANNUAL_WBAL] = stat[y][O_ANNUAL_PREC] - stat[y][O_ANNUAL_PET];
			stat[y][O_ANNUAL_WBALSEA] = GetWBALsea(m_weather[y], HamonPET);
			stat[y][O_ANNUAL_WRAT] = stat[y][O_ANNUAL_PET] > 0 ? stat[y][O_ANNUAL_PREC] / stat[y][O_ANNUAL_PET] : 0;
			stat[y][O_ANNUAL_CDD] = GetCoolingDD(m_weather[y], 0);

			//frost free period
			stat[y][O_ANNUAL_FSDAY] = Fp.Begin().GetJDay() + 1;
			stat[y][O_ANNUAL_FFDAY] = Fp.End().GetJDay() + 1;
			stat[y][O_ANNUAL_FPP] = Fp.GetLength();
			stat[y][O_ANNUAL_FDD] = m_weather[y].GetDD(0, Fp);

			//growing season period
			stat[y][O_ANNUAL_GSSDAY] = GSp.Begin().GetJDay() + 1;
			stat[y][O_ANNUAL_GSFDAY] = GSp.End().GetJDay() + 1;
			stat[y][O_ANNUAL_GSPP] = GSp.GetLength();
			stat[y][O_ANNUAL_GSDD] = m_weather[y].GetDD(0, GSp);
		}

		SetOutput(stat);


		return msg;
	}

	//daily coeficeint of varioation on temperature
	double CCCBioModel::GetTsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (int d = 0; d < weather.GetNbDay(); d++)
		{
			stat += weather.GetDay(d).GetTMean();
		}

		return stat[CFL::COEF_VAR];
	}

	double CCCBioModel::GetPsea(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (int d = 0; d < weather.GetNbDay(); d++)
		{
			stat += weather.GetDay(d).GetPpt();
		}

		return stat[CFL::COEF_VAR];
	}


	void CCCBioModel::GetPwv(std::vector<double>& Pwv, std::vector<int>& Pwm)
	{
		//int nbOcc[12] = {0};
		Pwv.resize(m_weather.GetNbYear());
		Pwm.resize(m_weather.GetNbYear());
		for (int y = 0; y < m_weather.GetNbYear(); y++)
		{
			//find the higest month
			double prcpMax = -1;
			int index = -1;
			for (int m = 0; m < 12; m++)
			{
				double prcp = m_weather[y][m].GetStat(STAT_PRCP, SUM);
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

	void CCCBioModel::GetPdv(std::vector<double>& Pdv, std::vector<int>& Pdm)
	{
		Pdv.resize(m_weather.GetNbYear());
		Pdm.resize(m_weather.GetNbYear());
		for (int y = 0; y < m_weather.GetNbYear(); y++)
		{
			//find the higest month
			double prcpMin = 999999999999;
			int index = -1;
			for (int m = 0; m < 12; m++)
			{
				double prcp = m_weather[y][m].GetStat(STAT_PRCP, SUM);
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
		for (int m = 0; m < 12; m++)
		{
			statTmin += weather[m].GetStat(H_TMIN)[MEAN];
			statTmax += weather[m].GetStat(H_TMAX)[MEAN];
		}

		return statTmax[HIGHEST] - statTmin[LOWEST];
	}

	double CCCBioModel::GetTsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (int m = 0; m < 12; m++)
		{
			stat += weather[m].GetStat(STAT_T_MN, MEAN);
		}

		return stat[CFL::COEF_VAR];
	}

	double CCCBioModel::GetPsea(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (int m = 0; m < 12; m++)
		{
			stat += weather[m].GetStat(STAT_PRCP, SUM);
		}

		return stat[CFL::COEF_VAR];
	}


	double CCCBioModel::GetPETsea(const CHamonPET& HamonPET)
	{
		CStatistic stat;
		for (int m = 0; m < 12; m++)
			stat += HamonPET.Get(m);

		return stat[CFL::COEF_VAR];
	}

	double CCCBioModel::GetWBALsea(const CWeatherYear& weather, const CHamonPET& HamonPET)
	{
		CStatistic stat;
		for (int m = 0; m < 12; m++)
			stat += (weather[m].GetStat(STAT_PRCP, SUM) - HamonPET.Get(m));

		return stat[CFL::COEF_VAR];
	}

	double CCCBioModel::GetCoolingDD(const CWeatherYear& weather, double threshold)
	{
		double FDD = 0;
		for (int d = 0; d < weather.GetNbDay(); d++)
		{
			if (weather.GetDay(d).GetTMean() < 0)
				FDD += -weather.GetDay(d).GetTMean();
		}

		return FDD;
	}

	double CCCBioModel::GetCoolingDD(const CWeatherMonth& weather, double threshold)
	{
		double FDD = 0;
		for (int d = 0; d < weather.GetNbDay(); d++)
		{
			if (weather.GetDay(d).GetTMean() < 0)
				FDD += -weather.GetDay(d).GetTMean();
		}

		return FDD;
	}

}