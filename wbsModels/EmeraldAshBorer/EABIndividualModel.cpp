//***********************************************************
// 07/07/2021	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "EABIndividualModel.h"
#include "ModelBase/EntryPoint.h"
////#include "Basic/DegreeDays.h"
//#include <boost/math/distributions/weibull.hpp>
//#include <boost/math/distributions/beta.hpp>
//#include <boost/math/distributions/Rayleigh.hpp>
//#include <boost/math/distributions/logistic.hpp>
//#include <boost/math/distributions/exponential.hpp>
//#include <boost/math/distributions/non_central_f.hpp>
//#include <boost/math/distributions/extreme_value.hpp>
//#include <boost/math/distributions/fisher_f.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::EAB;
using namespace std;


namespace WBSF
{
//	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	//enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	enum TOutput { O_PUPAE, O_ADULT, O_DEAD_ADULT, O_EMERGENCE, O_CUMUL_EMERGENCE, O_CUMUL_ADULT, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CAplanipennisModel::CreateObject);

	CAplanipennisModel::CAplanipennisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2022)";
	}

	CAplanipennisModel::~CAplanipennisModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CAplanipennisModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		if (parameters.size() == NB_PARAMS)
		{
			for (size_t p = 0; p < NB_PARAMS; p++)
				m_param[p] = parameters[c++].GetFloat();
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CAplanipennisModel::OnExecuteDaily()
	{
		ERMsg msg;

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	void CAplanipennisModel::ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output)
	{
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//int year = weather.GetTRef().GetYear();
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

		if (output.empty())
			output.Init(p, NB_OUTPUTS, 0);


		//Create stand
		CEABStand stand(this);
		stand.m_sigma = m_param[0];
		stand.m_psy_factor= m_param[1];

		//Create host
		CEABHostPtr pHost(new CEABHost(&stand));

		pHost->Initialize<CAplanipennis>(CInitialPopulation(p.Begin(), 0, 400, 100, PUPAE));

		//add host to stand			
		stand.m_host.push_front(pHost);

		
		

		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			if (output.IsInside(d))
				stand.GetStat(d, output[d]);

			stand.AdjustPopulation();
			HxGridTestConnection();
		}


		CStatistic stat = output.GetStat(O_EMERGENCE, p);
		if (stat.IsInit() && stat[SUM] > 0)
		{
			output[p.Begin()][O_CUMUL_EMERGENCE] = 0;
			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				output[TRef][O_CUMUL_EMERGENCE] = output[TRef - 1][O_CUMUL_EMERGENCE] + 100 * output[TRef][O_EMERGENCE] / stat[SUM];
		}

		stat = output.GetStat(O_ADULT, p);
		if (stat.IsInit() && stat[SUM] > 0)
		{
			output[p.Begin()][O_CUMUL_ADULT] = 0;
			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				output[TRef][O_CUMUL_ADULT] = output[TRef - 1][O_CUMUL_ADULT] + 100 * output[TRef][O_ADULT] / stat[SUM];
		}
	}

	enum TInput { I_CATCH, I_CUMUL_CATCH };
	void CAplanipennisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 4);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(2);
		obs.m_obs[I_CATCH] = stod(data[2]);//Catch
		obs.m_obs[I_CUMUL_CATCH] = stod(data[3]);//Catch

		m_years.insert(obs.m_ref.GetYear());


		m_SAResult.push_back(obs);
		
	}

	bool CAplanipennisModel::GetFValueDaily(CStatisticXY& stat)
	{


		double Nd = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_CATCH] > -999)
				Nd += m_SAResult[i].m_obs[I_CATCH];
		}


		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

			CModelStatVector output;
			ExecuteDaily(m_weather[year], output);

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//flies catch 
					if (m_SAResult[i].m_obs[I_CUMUL_CATCH] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_CUMUL_CATCH];
						double sim = output[m_SAResult[i].m_ref][O_CUMUL_ADULT];

						for (size_t ii = 0; ii < log(Nd); ii++)
							stat.Add(obs, sim);
					}
				}
			}//for all results

		}

		return true;

	}

}
