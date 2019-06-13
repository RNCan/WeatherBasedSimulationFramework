//**************************************************************************************************************
// 13/06/2019	1.0.0	Rémi Saint-Amant	Create from articles Vermunt 2011
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "UnderbarkTemperatureModel.h"
#include "UnderbarkTemperature.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CUnderbarkTemperatureModel::CreateObject);

	enum TColdHardinessH { O_TAIR, O_TBARK, NB_OUTPUTS_H };
	enum TColdHardinessD { O_TMIN, O_TMAX, O_TBARK_MIN, O_TBARK_MAX, NB_OUTPUTS_D };


	extern const char HEADER_H[] = "Tair,Tbark";
	extern const char HEADER_D[] = "Tmin,Tmax,Tbmin,Tbmax";


	CUnderbarkTemperatureModel::CUnderbarkTemperatureModel()
	{
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.0.0 (2019)";

		m_model = ASH_VERMUNT;
	}


	CUnderbarkTemperatureModel::~CUnderbarkTemperatureModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CUnderbarkTemperatureModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		m_model = parameters[c++].GetInt();

		//		if (m_model == PINE_REGNIERE && m_info.m_TM.Type() == CTRef::HOURLY)
			//		msg.ajoute("Mountain Pine (Regniere) does not support hourly output");

		return msg;
	}

	ERMsg CUnderbarkTemperatureModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();


		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		m_output.Init(p, NB_OUTPUTS_D, -999, HEADER_D);



		//if (m_model == ASH_VERMUNT)
		//{

		ASSERT(m_weather.IsHourly());

		CModelStatVector AshHourly;
		ExecuteHourly(AshHourly);

		//transform hourly results into daily results
		CTStatMatrix stats(AshHourly, CTM::DAILY);
		m_output.Init(stats.m_period, NB_OUTPUTS_D, -999, HEADER_D);

		for (CTRef TRef = stats.m_period.Begin(); TRef <= stats.m_period.End(); TRef++)
		{
			m_output[TRef][O_TMIN] = stats[TRef][O_TAIR][LOWEST];
			m_output[TRef][O_TMAX] = stats[TRef][O_TAIR][HIGHEST];
			m_output[TRef][O_TBARK_MIN] = stats[TRef][O_TBARK][LOWEST];
			m_output[TRef][O_TBARK_MAX] = stats[TRef][O_TBARK][HIGHEST];
		}

		return msg;
	}


	
	ERMsg CUnderbarkTemperatureModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		ExecuteHourly(m_output);

		return msg;
	}

	void CUnderbarkTemperatureModel::ExecuteHourly(CModelStatVector& output)
	{
		ASSERT(m_weather.IsHourly());

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::HOURLY);
		output.Init(p, NB_OUTPUTS_H, -999, HEADER_H);

		if (m_model == ASH_VERMUNT)
		{
			CNewtonianAshUnderbarkT AshModel(m_weather.GetHour(p.Begin())[H_TAIR]);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CHourlyData& w = m_weather.GetHour(TRef);
				double Tair = w[H_TAIR];
				double Tbark = AshModel.next_step(Tair);

				output[TRef][O_TAIR] = Tair;
				output[TRef][O_TBARK] = Tbark;
			}
		}
		else if (m_model == PINE_REGNIERE)
		{
			p = m_weather.GetEntireTPeriod(CTM::DAILY); 
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& w = m_weather.GetDay(TRef);
				CMountainPineUnderbarkT m(w);

				for (size_t h = 0; h < 24; h++)
				{
					CTRef TRefH = TRef.as(CTM::HOURLY) + h;
					double Tair = m_weather.GetHour(TRefH)[H_TAIR];
					double Tbark = m.GetTbark(h);

					output[TRefH][O_TAIR] = Tair;
					output[TRefH][O_TBARK] = Tbark;
				}
			}
		}



	}




}


//double CUnderbarkTemperatureModel::Tair2Tbark(TTUBark type, double Tair)
	//{
	//	double Tbark = Tair;

	//	//Annual bark temperature. Only valid lower -17. 
	//	if (Tair <= -17)
	//	{
	//		switch (type)
	//		{
	//		case LINEAR:Tbark = -2.38 + 0.747*Tair; break;
	//		case NONLINEAR:Tbark = -44.6 + 62.4*exp(0.0398*Tair); break;
	//		default: ASSERT(false);
	//		}
	//	}

	//	return Tbark;
	//}


