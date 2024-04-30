//*********************************************************************
//06/04/2024	1.0.0	Rémi Saint-Amant	Creation
// 
// Model based on article: 
// A.Santini (2004). Vegetative bud-burst variability of European elms
// 
// calibration for Ulmus americana based on data
// Smith (1915). Phenological dates and meteorological data recorded by Thomas Mikesell between 1873 and 1912 at Wauseon, Ohio.
//		We used "In blossom" column adjusted with "Buds start" column.
// Carol K.Augspurger (2009). Spring 2007 warmth and frost: phenology, damage and refoliation in a temperate deciduous forest 
// Calinger (2023). A century of climate warming results in growing season extension Delayed autumn leaf phenology in north central North America
//*********************************************************************
#include "BudBurstUlmus.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurstUlmusModel::CreateObject);



	enum TOutput { O_TT, O_CD, O_TTT, O_BUDBURST, NB_OUTPUTS };

	const array < array<double, NB_PARAMS >, NB_SPECIES> CBudBurstUlmusModel::P =
	{ {
		{0.0, 0.0, 122.2, 357.77, -0.012},//americana
		{5.0, 5.0, 66.93, 439.28, -0.033},//glabra
		{5.0, 5.0, 74.02, 434.00, -0.030},//laevis
		{5.0, 5.0, 74.55, 299.86, -0.025},//minor
	} };





	CBudBurstUlmusModel::CBudBurstUlmusModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2024)";
		m_P = P[AMERICANA];


		
	}



	CBudBurstUlmusModel::~CBudBurstUlmusModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CBudBurstUlmusModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();
		ASSERT(m_species < NB_SPECIES);

		
		m_P = P[m_species];
		if (parameters.size() == 1 + NB_PARAMS)
		{
			for (size_t i = 0; i < NB_PARAMS; i++)
				m_P[i] = parameters[c++].GetReal();
		}

		return msg;
	}






	//This method is call to compute solution
	ERMsg CBudBurstUlmusModel::OnExecuteDaily()
	{
		ERMsg msg;

		//if (!m_weather.IsHourly())
		//	m_weather.ComputeHourlyVariables();

		//compute input
		// Re sample Daily Climate


		//CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_P[P_T2], 50);
		//DDmodel.Execute(m_weather, m_DD);


		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::DAILY));
		m_output.Init(pp, NB_OUTPUTS, -999);


		for (size_t yy = 1; yy < m_weather.GetNbYears(); yy++)
		{
			ExecuteOneYear(yy, m_weather, m_output);
		}

		//3ExecuteAllYears(m_weather, m_output);


		return msg;
	}


	void CBudBurstUlmusModel::ExecuteOneYear(size_t yy, CWeatherYears& weather, CModelStatVector& output)const
	{
		if (output.empty())
		{
			int year = weather[yy].GetTRef().GetYear();
			CTPeriod pp(year - 1, JANUARY, DAY_01, year, DECEMBER, DAY_31);
			output.Init(pp, NB_OUTPUTS, -999);
		}

		//CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_P[P_T2], 50);
		//CModelStatVector DD;
		//DDmodel.Execute(weather, DD);


		int CD = 0;//ChillingDay
		double TT = 0;//TermalTime
		
		for (size_t yyy = 0; yyy < 2; yyy++)
		{
			size_t y = yy + yyy - 1;
			size_t m1 = yyy == 0 ? NOVEMBER : JANUARY;
			size_t m2 = yyy == 0 ? DECEMBER : OCTOBER;
			size_t dd = 0;

			for (size_t m = m1; m <= m2; m++)
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
				{
					CTRef TRef = weather[y][m][d].GetTRef();

					
					if (weather[y][m][d].GetStat(H_TAIR)[MEAN] < m_P[P_T1])
						CD++;

					if (yyy == 1 && m >= FEBRUARY)
					{
						TT += max(0.0, weather[y][m][d].GetStat(H_TAIR)[MEAN] - m_P[P_T2]);
						//TT += m_DD[TRef][0];
					}
					
					double TTT = m_P[P_A] + m_P[P_B] * exp(m_P[P_R] * CD);
					output[TRef][O_TTT] = TTT;
					output[TRef][O_BUDBURST] = TT / TTT;//TT < TTT ? 0 : 1;
				}
			}
		}
	}



	enum { I_SITE, I_YEAR, I_BB, NB_INPUTS1 };
	void CBudBurstUlmusModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		CSAResult obs;


		obs.m_ref.FromFormatedString(data[I_YEAR]);
		obs.m_obs[0] = stod(data[I_BB]);

		if (obs.m_obs[0] > -999)
		{
			m_years.insert(obs.m_ref.GetYear());
		}


		m_SAResult.push_back(obs);

	}



	bool CBudBurstUlmusModel::GetFValueDaily(CStatisticXY& stat)
	{
		//low and hi relative development rate must be approximatively the same
		//if (!IsParamValid())
			//return;


		//return CalibrateSDI(stat);
		if (!m_SAResult.empty())
		{


			if (m_data_weather.GetNbYears() == 0)
			{
				//CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_P[P_T2], 50);
				//DDmodel.Execute(m_weather, m_DD);


				CTPeriod pp(*m_years.begin() - 1, JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));

				((CLocation&)m_data_weather) = m_weather;
				//data_weather.SetHourly(false);
				m_data_weather.CreateYears(pp);

				for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
				{
					m_data_weather[year] = m_weather[year];
				}

				

			}

			CTPeriod pp(m_data_weather.GetEntireTPeriod(CTM::ANNUAL));
			//CModelStatVector output(pp, 1, -999);
			//map<int, CTRef> output;
			map<int, double> output;

			for (size_t yy = 1; yy < m_weather.GetNbYears(); yy++)
			{
				int year = m_weather[yy].GetTRef().GetYear();
				CModelStatVector tmp;
				ExecuteOneYear(yy, m_weather, tmp);

				CTRef TRef = tmp.GetFirstTRef(O_BUDBURST, ">=", 1, -1);
				if (TRef.IsInit())
				{
					ASSERT(tmp[TRef][O_BUDBURST] <= 1);
					ASSERT(tmp[TRef+1][O_BUDBURST] >= 1);
					output[year] = TRef.GetJDay() + 1 + (1 - tmp[TRef][O_BUDBURST]) / (tmp[TRef+1][O_BUDBURST] - tmp[TRef][O_BUDBURST]);
				}

				//output[year] = tmp.GetFirstTRef(O_BUDBURST, ">=", 1, 0);
			}

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				//ASSERT(output.IsInside(m_SAResult[i].m_ref));
				//if (output[m_SAResult[i].m_ref.GetYear()].IsInit()) 
				{
					double BB_obs = m_SAResult[i].m_obs[0];
					//double BB_sim = output[m_SAResult[i].m_ref.GetYear()].GetJDay() + 1;
					double BB_sim = output[m_SAResult[i].m_ref.GetYear()];
					stat.Add(BB_obs, BB_sim);
				}

			}//for all results
		//}
		}

		return true;
	}






}