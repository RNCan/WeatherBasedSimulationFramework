﻿//**************************************************************************************************************
// 20/03/2019	1.0.2	Rémi Saint-Amant	new equation of mortality based on logistic model
// 13/04/2018	1.0.1	Rémi Saint-Amant	Compile with VS 2017
// 08/05/2017	1.0.0	Rémi Saint-Amant	Create from articles Cuddington 2018
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "EmeraldAshBorerColdHardiness.h"
#include "TreeMicroClimate.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEmeraldAshBorerColdHardinessModel::CreateObject);

	enum TColdHardinessH { O_TAIR, O_TBARK, O_H_WT, O_H_SCP, O_H_MORTALITY, O_H_DIFF_TMIN_SCP, NB_OUTPUTS_H };
	enum TColdHardinessD { O_TMIN, O_TBARK_MIN, O_D_WT, O_D_SCP, O_D_MORTALITY, O_D_DIFF_TMIN_SCP, NB_OUTPUTS_D };
	enum TColdHardinessA { O_A_TMIN, O_A_TBARK_MIN_L, O_A_TBARK_MIN_N, O_A_MORTALITY_L, O_A_MORTALITY_N, NB_OUTPUTS_A };


	extern const char HEADER_H[] = "Tair,Tbark,wT,MDT,SCP,mortality";
	extern const char HEADER_D[] = "Tmin,Tmax,Tbmin,Tbmax,wT,MDT,SCP,mortality";
	extern const char HEADER_A[] = "TairMin,TbarkMinLinear,TbarkMinNonlinear,MortalityLinear,MortalityNonlinear";

	CEmeraldAshBorerColdHardinessModel::CEmeraldAshBorerColdHardinessModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.2 (2019)";


		m_n_Δt = 88;
		m_λ = 0.0635;
		m_wTº = 6.4;
		m_wTmin = -8.2;

		m_SCPᶫ = -37;
		m_SCPᴴ = -14;//by Venette
	}

	CEmeraldAshBorerColdHardinessModel::~CEmeraldAshBorerColdHardinessModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CEmeraldAshBorerColdHardinessModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		if (parameters.size() == 6)
		{
			m_n_Δt = parameters[c++].GetInt();
			m_λ = parameters[c++].GetReal();
			m_wTº = parameters[c++].GetReal();
			m_wTmin = parameters[c++].GetReal();
			m_SCPᶫ = parameters[c++].GetReal();
			m_SCPᴴ = parameters[c++].GetReal();
		}

		return msg;
	}

	ERMsg CEmeraldAshBorerColdHardinessModel::OnExecuteAnnual()
	{
		ERMsg msg;
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		ASSERT(m_weather.IsHourly());

		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUTS_A, -999, HEADER_A);
		for (size_t y = 1; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = CTPeriod(CTRef(m_weather[y - 1].GetTRef().GetYear(), SEPTEMBER, FIRST_DAY, FIRST_HOUR), CTRef(m_weather[y].GetTRef().GetYear(), APRIL, LAST_DAY, LAST_HOUR));

			CStatistic statA;
			statA += m_weather[y - 1].GetStat(H_TMIN, p);
			statA += m_weather[y].GetStat(H_TMIN, p);


			double Tmin = statA[LOWEST];
			m_output[y][O_A_TMIN] = Tmin;
			m_output[y][O_A_TBARK_MIN_L] = Tair2Tbark(LINEAR, Tmin);
			m_output[y][O_A_TBARK_MIN_N] = Tair2Tbark(NONLINEAR, Tmin);
			m_output[y][O_A_MORTALITY_L] = Tbark2MortalityLogistic(m_output[y][O_A_TBARK_MIN_L]) * 100;
			m_output[y][O_A_MORTALITY_N] = Tbark2MortalityLogistic(m_output[y][O_A_TBARK_MIN_N]) * 100;
		}

		return msg;
	}


	double get_w(int Δt, double a0)
	{
		double w = (1.0 / (1.0 + exp(-a0 * Δt)));

		return w;
	}

	double get_wT(CModelStatVector& output, CTRef TRef, int n_Δt, double a0)
	{
		assert(TRef.GetTM().Type() == CTM::HOURLY);
		assert(n_Δt > 0);

		CStatistic T;
		CStatistic W;

		for (int Δt = -n_Δt; Δt <= 0; Δt++)
		{
			double w = 1.0 / n_Δt;
			W += w;

			T += w * output[TRef + Δt][O_TBARK];
		}

		double wT = W[SUM] > 0 ? T[SUM] / W[SUM] : -999;

		return wT;
	}

	double get_SCP(double wT, double wTº, double λ, double SCPᶫ, double SCPᴴ)
	{
		ASSERT(SCPᶫ <= SCPᴴ);

		double ΔSCP = SCPᴴ - SCPᶫ;
		double SCP = SCPᶫ + ΔSCP * (1.0 / (1.0 + exp(-λ * (wT - wTº))));

		return SCP;
	}


	void CEmeraldAshBorerColdHardinessModel::ExecuteDaily(CModelStatVector& outputD)
	{
		CModelStatVector outputH;
		ExecuteHourly(outputH);


		//transform hourly results into daily results
		CTStatMatrix stats(outputH, CTM::DAILY);
		outputD.Init(stats.m_period, NB_OUTPUTS_D, -999, HEADER_D);

		for (CTRef TRef = stats.m_period.Begin(); TRef <= stats.m_period.End(); TRef++)
		{
			outputD[TRef][O_TMIN] = stats[TRef][O_TAIR][LOWEST];
			outputD[TRef][O_TBARK_MIN] = stats[TRef][O_TBARK][LOWEST];
			outputD[TRef][O_D_WT] = stats[TRef][O_H_WT][MEAN];
			outputD[TRef][O_D_SCP] = stats[TRef][O_H_SCP][MEAN];
			outputD[TRef][O_D_MORTALITY] = stats[TRef][O_H_MORTALITY][MEAN];
			if (stats[TRef][O_TBARK].IsInit() && stats[TRef][O_H_SCP].IsInit())
				outputD[TRef][O_D_DIFF_TMIN_SCP] = stats[TRef][O_TBARK][LOWEST] - stats[TRef][O_H_SCP][LOWEST];
		}

	}

	CTRef GetLoTRef(CTPeriod p, CModelStatVector& output)
	{
		//get the minimum date pof the 28 days's mean
		CTRef loTRef;
		double lo_wT = 999;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double wT = get_wT(output, TRef, 672, 0);
			if (wT < lo_wT)
			{
				lo_wT = wT;
				loTRef = TRef;
			}
		}

		return loTRef;
	}

	double CEmeraldAshBorerColdHardinessModel::Tair2Tbark(TTUBark type, double Tair)
	{
		double Tbark = Tair;

		//Annual bark temperature. Only valid lower -17. 
		if (Tair <= -17)
		{
			switch (type)
			{
			case LINEAR:Tbark = -2.38 + 0.747*Tair; break;
			case NONLINEAR:Tbark = -44.6 + 62.4*exp(0.0398*Tair); break;
			default: ASSERT(false);
			}
		}

		return Tbark;
	}

	double CEmeraldAshBorerColdHardinessModel::Tbark2MortalityLogistic(double Tbark)
	{
		//calibrated with R by nls
		static const double k = 2.013E-4;
		static const double x0 = -2.418E4;

		return 1.0 - 1.0 / (1.0 + exp(-(k * (pow(Tbark, 3.0) - x0))));
	}





	void CEmeraldAshBorerColdHardinessModel::ExecuteHourly(CModelStatVector& output)
	{
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		ASSERT(m_weather.IsHourly());

		output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS_H, -999, HEADER_H);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::HOURLY);

		//first step: compute Tair and Tbark from regression
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			const CHourlyData& w = m_weather.GetHour(TRef);
			double Tair = w[H_TAIR];
			double Tbark = Tair2Tbark(NONLINEAR, Tair);//use nonlinear by default
			output[TRef][O_TAIR] = Tair;
			output[TRef][O_TBARK] = Tbark;
		}



		//second step: compute SCP and mortality
		for (size_t y = 0; y < p.GetNbYears() - 1; y++)
		{
			CTPeriod p2 = CTPeriod(CTRef(m_weather[y].GetTRef().GetYear(), SEPTEMBER, FIRST_DAY, FIRST_HOUR), CTRef(m_weather[y + 1].GetTRef().GetYear(), APRIL, LAST_DAY, LAST_HOUR));
			CTRef lowTRef = GetLoTRef(p2, output);

			double cur_wT = get_wT(output, p2.Begin(), m_n_Δt, 0);
			double cur_SCP = m_SCPᴴ;
			double ΣwT = 0;
			double mortality = 0;

			TPhase phase = ACCLIMATATION;

			for (CTRef TRef = p2.Begin(); TRef <= p2.End(); TRef++)
			{
				double wT = get_wT(output, TRef, m_n_Δt, 0);
				wT = max(m_wTmin, wT);

				double SCP = get_SCP(wT, m_wTº, m_λ, m_SCPᶫ, m_SCPᴴ);
				cur_SCP = (phase == ACCLIMATATION) ? min(cur_SCP, SCP) : max(cur_SCP, SCP);

				mortality = max(mortality, Tbark2MortalityLogistic(output[TRef][O_TBARK]));

				if (phase == ACCLIMATATION && TRef >= lowTRef)
				{
					phase = DESACCLIMATATION;
				}
				output[TRef][O_H_WT] = wT;
				output[TRef][O_H_SCP] = cur_SCP;
				output[TRef][O_H_MORTALITY] = mortality * 100;
				output[TRef][O_H_DIFF_TMIN_SCP] = output[TRef][O_TBARK] - output[TRef][O_H_SCP];
			}//for all hours during the winter
		}//for all years
	}




	//*****************************************************************************************************************
	//Next 4 methods are for Simulated Annealing
	enum TInput { I_SITE, I_YEAR, I_MONTH, I_DAY };
	enum TInputCrosthwaite { Cr_SCP = I_DAY + 1, Cr_SCP_SME, NB_CROSTHWAITE_COLUMNS };
	enum TInputChistianson { Ch_N = I_DAY + 1, Ch_SCP_MIN, CH_SCP_MEDIAN, Ch_SCP_MEAN, Ch_SCP_SME, NB_CHRISTIANSON_COLUMNS };


	void CEmeraldAshBorerColdHardinessModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == 3)
		{
			std::vector<double> obs(3);
			for (size_t i = 0; i < obs.size(); i++)
				obs[i] = ToDouble(data[i]);

			m_SAResult.push_back(CSAResult(CTRef(0, 0, 0, 0, CTM::ATEMPORAL), obs));
		}
		else if (header.size() == NB_CROSTHWAITE_COLUMNS)
		{
			CTRef TRef(ToInt(data[I_YEAR]), ToSizeT(data[I_MONTH]) - 1, ToSizeT(data[I_DAY]) - 1);
			std::vector<double> obs(NB_CROSTHWAITE_COLUMNS);

			for (size_t i = 0; i < obs.size(); i++)
				obs[i] = ToDouble(data[i]);

			m_SAResult.push_back(CSAResult(TRef, obs));
		}
		else if (header.size() == NB_CHRISTIANSON_COLUMNS)
		{
			std::vector<double> obs(NB_CHRISTIANSON_COLUMNS);

			for (size_t i = 0; i < obs.size(); i++)
				obs[i] = ToDouble(data[i]);

			CTRef TRef(ToInt(data[I_YEAR]), ToSizeT(data[I_MONTH]) - 1, ToSizeT(data[I_DAY]) - 1);
			m_SAResult.push_back(CSAResult(TRef, obs));
		}
	}

	double CEmeraldAshBorerColdHardinessModel::logistic(double x, double k, double x0)
	{
		return 1.0 - 1.0 / (1.0 + exp(-(k * (pow(x, 3.0) - x0))));
	}
	double CEmeraldAshBorerColdHardinessModel::Logistic(double x, double K, double A, double R, double x0)
	{
		return (K*(1 / (1 + A * exp(-R * (x - x0)))));
	}
	double CEmeraldAshBorerColdHardinessModel::Weibull(double x, double k, double y, double x0)
	{
		return exp(-pow(max(0.0, (x - x0)) / y, k));
	}
	double CEmeraldAshBorerColdHardinessModel::SShaped(double x, double p, double k, double x0)
	{
		return (1.0 - 1.0 / (1.0 + exp(-k * pow(x - x0, p))));
	}

	bool CEmeraldAshBorerColdHardinessModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();


		if (m_SAResult.size() > 0)
		{
			if (m_SAResult[0].m_obs.size() == 3)
			{
				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					double obsV = m_SAResult[i].m_obs[2];
					double simV = logistic(m_SAResult[i].m_obs[1], m_λ, m_wTº);
					stat.Add(obsV, simV);
				}
				return true;
			}




			//now compare simulation with observation data
			if (m_SAResult[0].m_obs.size() == NB_CROSTHWAITE_COLUMNS)
			{
				CModelStatVector sim;
				ExecuteDaily(sim);

				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					if (m_SAResult[i].m_obs[Cr_SCP] > -999 &&
						sim.IsInside(m_SAResult[i].m_ref))
					{
						double obsV = m_SAResult[i].m_obs[Cr_SCP];
						double simV = sim[m_SAResult[i].m_ref][O_D_SCP];

						stat.Add(simV, obsV);
					}
				}

			}
		}

		return true;
	}
}