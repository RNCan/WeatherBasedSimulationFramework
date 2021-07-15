//**********************************************************************
//
// 02/06/2021 Rémi Saint-Amant	Update to BioSIM 11
// 02/03/2011 Rémi Saint-Amant	New build 
// 08/04/2011 Rémi Saint-Amant	Create a specific class for spruce budworm
// 01/07/2010 Rémi St-Amant and J. Régnière	Generalization to Continuing Ratio Model
//
//Eldon
//**********************************************************************

#include "SBW-Eldon-Model.h"
#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSBWEldonModel::CreateObject);


	enum TOutput { O_DD, O_AI, NB_OUTPUTS };


	CSBWEldonModel::CSBWEldonModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2021)";
	}

	CSBWEldonModel::~CSBWEldonModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CSBWEldonModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//int c = 0;
		//m_species = parameters[c++].GetInt();

		return msg;
	}


	ERMsg CSBWEldonModel::OnExecuteDaily()
	{
		//if (!m_weather.IsHourly())
		//	m_weather.ComputeHourlyVariables();



		return ExecuteModel(m_output);
	}



	typedef pair<double, double> ELDON_t;
	typedef array<ELDON_t,13> ELDON_array;
	static const ELDON_array ELDON_CHART =
	{ {
		{-DBL_MAX, 2.0},
		{38, 2.0},
		{62, 2.5},
		{162, 3.0},
		{200, 3.5},
		{237, 4.0},
		{272, 4.5},
		{306, 5.0},
		{360, 5.5},
		{413, 6.0},
		{474, 6.5},
		{535, 7.0},
		{DBL_MAX, 7.0},
	} };

	double findAI(double DD) 
	{
		ELDON_array::const_iterator it2 = std::lower_bound(ELDON_CHART.begin(), ELDON_CHART.end(), ELDON_t(DD,0));
		ELDON_array::const_iterator it1 = it2 - 1;
		assert(it1 != ELDON_CHART.end() && it2 != ELDON_CHART.end());
		//if(it == ELDON_CHART.end())
			//return 2;
		
		return it1->second + (DD - it1->first)*(it2->second - it1->second) / (it2->first - it1->first);
	}

	ERMsg CSBWEldonModel::ExecuteModel(CModelStatVector& stat)
	{
		ERMsg msg;


		static const double BASE_TEMP = 5.56;



		stat.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUTS);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);
			CTRef start_date(p.Begin().GetYear(), MARCH, DAY_01);
			double DD_sum = 0.0;

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& day = m_weather.GetDay(TRef);
				if (TRef >= start_date)
				{
					if (day[H_TMAX][MEAN] > BASE_TEMP)
					{
						double Tmean = (day[H_TMAX][MEAN] + day[H_TMIN][MEAN]) / 2.0;
						double alpha = (day[H_TMAX][MEAN] - day[H_TMIN][MEAN]) / 2.0;

						double theta = (day[H_TMIN][MEAN] > BASE_TEMP) ? -PI / 2.0 : asin(max(-1.0, min(1.0, 1.0 / alpha * (BASE_TEMP - Tmean))));
						DD_sum += 1.0 / PI * ((Tmean - BASE_TEMP)*(PI / 2 - theta) + alpha * cos(theta));
					}
				}

				double AI = findAI(DD_sum);

				size_t c = 0;
				stat[TRef][c++] = WBSF::Round(DD_sum, 1);
				stat[TRef][c++] = WBSF::Round(AI, 1);
			}
		}

		return msg;
	}



}