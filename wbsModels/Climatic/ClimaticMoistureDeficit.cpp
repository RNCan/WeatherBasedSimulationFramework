//**********************************************************************
// 01/11/2023	1.0		Rémi Saint-Amant	Creation
// From: Dissecting indices of aridity for assessing the impacts of global climate change 
// Evan H.Girvetz & Chris Zganjar
// Climatic Change (2014) 126:469–483
// DOI 10.1007 / s10584 - 014 - 1218 - 9
//**********************************************************************

#include "ClimaticMoistureDeficit.h"
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
		CModelFactory::RegisterModel(CClimaticMoistureDeficitModel::CreateObject);

	//Contructor
	CClimaticMoistureDeficitModel::CClimaticMoistureDeficitModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2023)";

		m_nb_years = 1;//Annual;
	}

	CClimaticMoistureDeficitModel::~CClimaticMoistureDeficitModel()
	{}

	enum TEToOutput { NB_ETo_OUTPUTS = 1 };
	//ETo : monthly reference evapotranspiration (mm)
	//PRCP : monthly precipitation (mm)
	//CMD : monthly climatic moisture deficit
	//CMS : monthly climatic moisture surplus
	enum TMonthlyOutput { O_TAIR, O_PRCP, O_ETo, O_CMD, O_CMS, NB_OUTPUTS };


	//this method is call to load your parameter in your variable
	ERMsg CClimaticMoistureDeficitModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		int c = 0;
		/*m_nb_years = parameters[c++].GetInt();
		if (m_nb_years == 0)
			m_nb_years = m_weather.size();

		if (m_nb_years > m_weather.size())
			msg.ajoute("Invalid input parameters: number of years must be lesser than the number of weather years.");*/


		return msg;
	}


	ERMsg CClimaticMoistureDeficitModel::OnExecuteAnnual()
	{
		ASSERT(m_nb_years <= m_weather.size());

		ERMsg msg;

		CModelStatVector ETo = GetETo(m_weather);

		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUTS, -999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CStatistic A_T;
			CStatistic A_Eto;
			CStatistic A_prcp;
			CStatistic CMD;
			CStatistic CMS;

			int year = m_weather.GetFirstYear() + int(y);
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				CStatistic M_T;
				CStatistic M_Eto;
				CStatistic M_prcp;

				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef TRef(year, m, d);
					M_T += m_weather[y][m][d][H_TAIR][MEAN];
					M_Eto += ETo[TRef][0];
					M_prcp += m_weather[y][m][d][H_PRCP][SUM];
				}

				double M_EtoSum = M_T[MEAN] >= 0 ? M_Eto[SUM] : 0;
				A_T += M_T[MEAN];
				A_Eto += M_EtoSum;
				A_prcp += M_prcp[SUM];

				CMD += max(0.0, M_EtoSum - M_prcp[SUM]);
				CMS += max(0.0, M_prcp[SUM] - M_EtoSum);
			}

			m_output[y][O_TAIR] = A_T[MEAN];
			m_output[y][O_PRCP] = A_prcp[SUM];
			m_output[y][O_ETo] = A_Eto[SUM];
			m_output[y][O_CMD] = CMD[SUM];
			m_output[y][O_CMS] = CMS[SUM];
		}


		return msg;
	}

	ERMsg CClimaticMoistureDeficitModel::OnExecuteMonthly()
	{
		ASSERT(m_nb_years <= m_weather.size());

		ERMsg msg;

		CModelStatVector ETo = GetETo(m_weather);

		m_output.Init(m_weather.GetEntireTPeriod(CTM::MONTHLY), NB_OUTPUTS, -999);


		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				CStatistic M_T;
				CStatistic M_ETo;
				CStatistic M_prcp;
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef TRef(year, m, d);
					M_T += m_weather[y][m][d][H_TAIR][MEAN];
					M_ETo += ETo[TRef][0];
					M_prcp += m_weather[y][m][d][H_PRCP][SUM];
				}

				m_output[y * 12 + m][O_TAIR] = M_T[MEAN];
				m_output[y * 12 + m][O_ETo] = M_T[MEAN] >= 0 ? M_ETo[SUM] : 0;
				m_output[y * 12 + m][O_PRCP] = M_prcp[SUM];
				m_output[y * 12 + m][O_CMD] = max(0.0, m_output[y * 12 + m][O_ETo] - m_output[y * 12 + m][O_PRCP]);
				m_output[y * 12 + m][O_CMS] = max(0.0, m_output[y * 12 + m][O_PRCP] - m_output[y * 12 + m][O_ETo]);
			}
		}

		return msg;
	}

	CModelStatVector CClimaticMoistureDeficitModel::GetETo(const CWeatherStation& weather)
	{
		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		CModelStatVector ETo(p, 1, -999);


		double lat = m_info.m_loc.m_lat * PI / 180.0;//latitude in radian

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			const CWeatherDay& wDay = weather.GetDay(TRef);
			double J = TRef.GetJDay() + 1;
			double solar = 0.409 * sin(2 * PI * J / 365 - 1.39);
			double Ws = acos(-tan(lat) * tan(solar));
			double Dr = 1 + 0.033 * cos(2 * PI * J / 365);

			static const double Gsc = 0.0829; //MJ/(m² day)
			double Ra = 0.408 * 24 * 60 * Gsc * Dr * (Ws * sin(lat) * sin(solar) + cos(lat) * cos(solar) * sin(Ws)) / PI;
			double Eto = 0.0023 * (wDay[H_TAIR][MEAN] + 17.8) * pow(wDay[H_TMAX][MEAN] - wDay[H_TMIN][MEAN], 0.5) * Ra;

			ETo[TRef][0] = Eto;
		}

		return ETo;
	}


	//MAT : mean annual air temperature
	double CClimaticMoistureDeficitModel::MAT(const CWeatherYear& weather)
	{
		return weather.GetStat(H_TNTX)[MEAN];
	}

	//MCMT : mean coldest month temperature
	double CClimaticMoistureDeficitModel::MCMT(const CWeatherYear& weather)
	{
		double minT = 999;
		for (size_t m = 0; m < 12; m++)
		{
			double T = weather[m].GetStat(H_TNTX)[MEAN];
			if (T < minT)
				minT = T;
		}

		return minT;
	}

	//EMT : extreme minimum temperature
	double CClimaticMoistureDeficitModel::EMT(const CWeatherYear& weather)
	{
		return weather.GetStat(H_TMIN)[LOWEST];
	}

	//MWMT : mean warmest month temperature
	double CClimaticMoistureDeficitModel::MWMT(const CWeatherYear& weather)
	{
		double maxT = -999;
		for (size_t m = 0; m < 12; m++)
		{
			double T = weather[m].GetStat(H_TNTX)[MEAN];
			if (T > maxT)
				maxT = T;
		}

		return maxT;
	}

	//NFFD: Total number of frost free day
	double CClimaticMoistureDeficitModel::NFFD(const CWeatherYear& weather)
	{
		size_t NFFD = 0;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			double T = weather.GetDay(d)[H_TMIN][LOWEST];
			if (T > 0)
				NFFD++;
		}

		return NFFD;
	}



	//CHDD0 : chilling degree days under 0 (from August first)
	double CClimaticMoistureDeficitModel::CHDD(const CWeatherYear& weather, double threshold)
	{
		int year = weather.GetTRef().GetYear();
		CTPeriod pp(CTRef(year, AUGUST, DAY_01), CTRef(year, DECEMBER, DAY_31));

		double FDD = 0;
		for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
		{
			double Tair = weather.GetDay(TRef)[H_TNTX][MEAN];
			if (Tair < threshold)
				FDD += -(Tair - threshold);
		}


		return FDD;
	}



	double CClimaticMoistureDeficitModel::CDD5(const CWeatherYear& weather, double threshold)
	{
		CDegreeDays model(CDegreeDays::DAILY_AVERAGE, threshold);

		CModelStatVector output;
		model.Execute(weather, output);

		//CTTransformation TT(output.GetTPeriod(), CTM::ANNUAL);
		//CTStatMatrix stats(output, TT);

		CTPeriod pp = output.GetTPeriod();

		double CDD = 0;
		for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
			CDD += output[TRef][CDegreeDays::S_DD];


		return CDD;// stats[CDegreeDays::S_DD][SUM];
	}



	double CClimaticMoistureDeficitModel::PPT5(const CWeatherYear& weather)
	{
		double sumP = 0;
		for (size_t m = MAY; m <= SEPTEMBER; m++)
		{
			sumP += weather[m].GetStat(H_PRCP)[SUM];

		}

		return sumP;
	}
}