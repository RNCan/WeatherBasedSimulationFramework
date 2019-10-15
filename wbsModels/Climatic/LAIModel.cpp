//**********************************************************************
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "LAIModel.h"
#include "LAI.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LeafAreaIndex;

namespace WBSF
{
	
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CLAIModel::CreateObject);


	enum TDailyOutput { O_LAI, NB_OUTPUTS };
	enum TOtherOutput { O_LAI_MIN, O_LAI_MEAN, O_LAI_MAX, NB_OTHER_OUTPUTS };

	//Constructor
	CLAIModel::CLAIModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2019)";

		m_forestCoverType = MIXED_WOODS;
		m_quantile = 0.5;//median
	}

	CLAIModel::~CLAIModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CLAIModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		size_t c = 0;
		
		m_forestCoverType = parameters[c++].GetInt();
		m_quantile = parameters[c++].GetReal();

		ASSERT(m_forestCoverType < NB_FOREST_COVER_TYPE);
		ASSERT(m_quantile > 0 && m_quantile < 1);

		return msg;
	}


	ERMsg CLAIModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OTHER_OUTPUTS, -9999);

		double Tsoil = 0;//
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CStatistic stat = GetLAI(m_weather[y], m_forestCoverType, m_quantile);

			m_output[y][O_LAI_MIN] = stat[LOWEST];
			m_output[y][O_LAI_MEAN] = stat[MEAN];
			m_output[y][O_LAI_MAX] = stat[HIGHEST];
		}

		return msg;
	}

	ERMsg CLAIModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_OTHER_OUTPUTS, -9999);

		double Tsoil = 0;
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				CStatistic stat = GetLAI(m_weather[y][m], m_forestCoverType, m_quantile);

				m_output[y * 12 + m][O_LAI_MIN] = stat[LOWEST];
				m_output[y * 12 + m][O_LAI_MEAN] = stat[MEAN];
				m_output[y * 12 + m][O_LAI_MAX] = stat[HIGHEST];

			}
		}


		return msg;
	}


	ERMsg CLAIModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, -9999);


		for (size_t y = 0; y < m_weather.size(); y++)
		{
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef ref = m_weather[y][m][d].GetTRef();
					double LAI = CLeafAreaIndex::ComputeLAI(ref.GetJDay(), m_forestCoverType, m_quantile);
					m_output[ref][O_LAI] = LAI;
				}
			}
		}


		return msg;
	}

	



	CStatistic CLAIModel::GetLAI(const CWeatherYear& weather, size_t forestCoverType, double quantile)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetLAI(weather[m], forestCoverType, quantile);

		return stat[MEAN];
	}

	CStatistic CLAIModel::GetLAI(const CWeatherMonth& weather, size_t forestCoverType, double quantile)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
		{
			double LAI = CLeafAreaIndex::ComputeLAI(weather.GetTRef().GetJDay(), forestCoverType, quantile);
			stat += LAI;
		}

		return stat;
	}

	
	
}